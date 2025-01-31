/*
 *------------------------------------------------------------------
 * Copyright (c) 2020 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *------------------------------------------------------------------
 */

#include <stdlib.h>
#include <sys/types.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>

#include <libmemif.h>
#include <common.h>
#include <sys/time.h>
#include <assert.h>

struct timeval start, end;

#define APP_NAME "loopback_example"
#define IF0_NAME "lo0"
#define IF1_NAME "lo1"

memif_connection_t intf0, intf1;
int is_reverse;
int epfd;
int cnt = 0, Time;

int
packet_generator (memif_connection_t *c, uint16_t num_pkts)
{
  int i, bi = 0;
  memif_buffer_t *mb;

  for (i = 0; (i < num_pkts) && (bi < c->tx_buf_num); i++)
    {
      mb = &c->tx_bufs[bi++];
      memset (mb->data, 1, mb->len);
    }

  return 0;
}

int
my_generator (memif_connection_t *c, uint16_t num_pkts, int cnt)
{
  int i, bi = 0;
  memif_buffer_t *mb;

  for (i = 0; (i < num_pkts) && (bi < c->tx_buf_num); i++)
    {
      mb = &c->tx_bufs[bi++];
      memset (mb->data, 0, mb->len);
    }
  ((int*)(&c->tx_bufs[0].data))[0] = cnt;

  return 0;
}

/* informs user about connected status. private_ctx is used by user to identify
 * connection */
int
on_connect (memif_conn_handle_t conn, void *private_ctx)
{
  int err;
  memif_connection_t *c = (memif_connection_t *) private_ctx;
  memif_conn_args_t *arg = (memif_conn_args_t *) ((char *)c->conn + sizeof(uint16_t));

  INFO ("memif connected! %d @%s@", (c == &intf1), (char*)arg->interface_name);

  c->is_connected = 1;
  alloc_memif_buffers (c);

  err = memif_refill_queue (conn, 0, -1, 0);
  if (err != MEMIF_ERR_SUCCESS)
    {
      INFO ("memif_refill_queue: %s", memif_strerror (err));
      return err;  
    }

  print_memif_details (c);
  gettimeofday(&start, NULL); // 记录开始时间

  /* Once both interfaces are connected send a test packet, master -> slave.
   * Slave will use zero-copy method to reply the same pakcet back.
   * (Configured by assigning responder_zero_copy as on_interrupt callback.)
   */
  // if ((intf0.is_connected == 1) && (intf1.is_connected == 1))
  //   {
  //     INFO("send inf0 %d", intf0.is_connected);
  //     send_packets (is_reverse ? &intf1 : &intf0, 0, packet_generator, MAX_MEMIF_BUFS,
	// 	    32 * 1024);
  //   }
  // INFO("after send check.");
  // sleep(5);
  // INFO("send inf0 %d", intf0.is_connected);
  // send_packets (&intf0, 0, packet_generator, 1, 2 * 1024);

  return 0;
}

#include <unistd.h>

/* informs user about disconnected status. private_ctx is used by user to
 * identify connection */
int
on_disconnect (memif_conn_handle_t conn, void *private_ctx)
{
  INFO ("memif disconnected!");

  memif_connection_t *c = (memif_connection_t *) private_ctx;

  c->is_connected = 0;
  cnt = 0;

//   for (int i=1;i<=8;i++){
//     sleep(1);
//     INFO ("sleep %d seconds.", i);
//   }

  /* stop event polling thread */
  // free_memif_buffers (c);
  // int err = memif_cancel_poll_event (memif_get_socket_handle (conn));
  // if (err != MEMIF_ERR_SUCCESS)
  //   INFO ("We are doomed...");

  return 0;
}

int
verify_packet (memif_conn_handle_t conn, void *private_ctx, uint16_t qid)
{
  memif_connection_t *c = (memif_connection_t *) private_ctx;
  int err = 0;
  void *want;

  err = memif_rx_burst (conn, qid, c->rx_bufs, MAX_MEMIF_BUFS, &c->rx_buf_num);
  if (err != MEMIF_ERR_SUCCESS)
    {
      INFO ("meif_rx_burst: %s", memif_strerror (err));
      return err;
    }
  // INFO ("c->rx_buf_num=%d", c->rx_buf_num);

  // refill the queue
  err = memif_refill_queue (conn, qid, c->rx_buf_num, 0);
  if (err != MEMIF_ERR_SUCCESS)
    INFO ("memif_refill_queue: %s", memif_strerror (err));

  // INFO("rx_buf_num:%d", c->rx_buf_num);
  char *a = (char *)(c->rx_bufs[0].data);
  if (a[0] == 'O' && a[1] == 'K'){ // first send
    cnt++;
    send_packets2 (&intf0, 0, packet_generator, 1, 1 * 1024, -cnt);
  }
  else if (((int *)c->rx_bufs[0].data)[0] == cnt){
    // if (c->rx_buf_num != 1024){
    //   INFO ("c->rx_buf_num=%d", c->rx_buf_num);
    // }
    assert(c->rx_buf_num == 1024);
    assert(((int *)c->rx_bufs[0].data)[cnt % (1024 / 4)] == cnt);

    // memif_buffer_t *mb;
    // int flag = 0;
    // for (int i = 0; i < c->rx_buf_num; i++)
    // {
    //   // memcpy (c->tx_bufs[i].data, c->rx_bufs[i].data, c->rx_bufs[i].len);
    //   	mb = &c->tx_bufs[i];
    //     int *a = (int *)mb->data;
    //     for (int j = 0; j < mb->len / sizeof(int); j++)
	  //       if (a[j] != cnt) flag = 1;
    //     // a[0] = flag;
    //     // a[flag % (need_size / 4)] = flag;
    // }
    // assert(flag == 0);

    cnt ++;
    send_packets2 (&intf0, 0, packet_generator, 1, 1 * 1024, -cnt);

  }
  else{
    INFO("error data:%d cnt:%d", ((int *)c->rx_bufs[0].data)[0], cnt);
  }

  // INFO("cnt:%d", cnt);
  if (cnt % 20000 == 0) {
    
    gettimeofday(&end, NULL); // 记录结束时间
    double seconds = (end.tv_sec - start.tv_sec) + 
    (end.tv_usec - start.tv_usec) / 1000000.0;
    INFO("cnt:%d avg_speed: %.6lf Gbps", cnt, cnt / 1000.0 * (1024.0/1024.0) / seconds * 8.0);
  }
  
//   want = malloc (c->rx_bufs[0].len);
//   if (want == NULL)
//     {
//       INFO ("Out of memory");
//       goto done;
//     }

//   memset (want, 0, c->rx_bufs[0].len);

//   err = memcmp (c->rx_bufs[0].data, want, c->rx_bufs[0].len);
//   if (err != 0)
//     {
//       INFO ("Received malformed data. ret: %d", err);
//     }
//   else
//     {
//       INFO ("Received correct data.");
//     }

// done:
//   err = memif_refill_queue (conn, qid, c->rx_buf_num, 0);
//   if (err != MEMIF_ERR_SUCCESS)
//     INFO ("memif_refill_queue: %s", memif_strerror (err));

//   /* stop polling and exit the program */
//   INFO ("Stopping the program");
//   err = memif_cancel_poll_event (memif_get_socket_handle (conn));
//   if (err != MEMIF_ERR_SUCCESS)
//     INFO ("We are doomed...");

  return err;
}

int
create_memif_interface (memif_socket_handle_t memif_socket,
			const char *if_name, int id, uint8_t is_master,
			memif_connection_t *ctx)
{
  memif_conn_args_t memif_conn_args = { 0 };
  int err;

  memif_conn_args.socket = memif_socket;
  memif_conn_args.interface_id = id;
  strncpy (memif_conn_args.interface_name, if_name,
	   sizeof (memif_conn_args.interface_name));
  memif_conn_args.is_master = is_master;
  err = memif_create (&ctx->conn, &memif_conn_args, on_connect, on_disconnect,
		      is_master ? verify_packet : responder_zero_copy,
		      (void *) ctx);
  if (err != MEMIF_ERR_SUCCESS)
    {
      INFO ("memif_create_socket: %s", memif_strerror (err));
      return err;
    }

  return 0;
}

void
print_help ()
{
  printf ("LIBMEMIF EXAMPLE APP: %s", APP_NAME);
#ifdef ICMP_DBG
  printf (" (debug)");
#endif
  printf ("\n");
  printf ("==============================\n");
  printf ("libmemif version: %s", LIBMEMIF_VERSION);
#ifdef MEMIF_DBG
  printf (" (debug)");
#endif
  printf ("\n");

  printf ("memif version: %s\n", memif_get_version_str ());
  printf ("==============================\n");
  printf ("In this example, two memif endpoints are connected to create a "
	  "loopback.\n");
  printf ("Once connected, a test packet is sent out the memif master "
	  "interface to\n");
  printf (
    "the memif slave interface, which replies with the same packet in a\n");
  printf ("zero-copy way.\n");
  printf (
    "In reverse mode, the packet is sent from the slave interface and is\n");
  printf ("looped back by the master interface.\n");
  printf ("==============================\n");
  printf ("Usage: loopback [OPTIONS]\n\n");
  printf ("Options:\n");
  printf ("\t-r\tReverse mode, verification packet is sent by slave.\n");
  printf ("\t-?\tShow help and exit.\n");
  printf ("\t-v\tShow libmemif and memif version information and exit.\n");
}

char socket_path[1024];

#include <sys/stat.h>
// Function to check if a path starts with '@'
int starts_with_at(const char *path) {
    if (path[0] == '@') {
        return 1;
    } else {
        return 0;
    }
}

// Function to check if a file exists
int file_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return 1;
    } else {
        return 0;
    }
}

// Function to delete a file
void delete_file(const char *path) {
    if (remove(path) == 0) {
        printf("File '%s' deleted successfully.\n", path);
    } else {
        perror("Failed to delete file:");
    }
}

#include<signal.h>
void check_remove_file(int signal) {
    // 检查 socket_path 是否以 '@' 开头
    if (!starts_with_at(socket_path)) {
        // 检查文件是否存在
        if (file_exists(socket_path)) {
            // 删除文件
            delete_file(socket_path);
        } else {
            printf("文件 '%s' 不存在。\n", socket_path);
        }
    }

    // 使用接收到的相同信号退出程序
    exit(signal);
}

int
main (int argc, char *argv[])
{
  signal(SIGINT, check_remove_file);

  memif_socket_args_t memif_socket_args = { 0 };
  memif_socket_handle_t memif_socket;
  int opt, err, ret = 0;
  is_reverse = 0;

  while ((opt = getopt (argc, argv, "r?vn:")) != -1)
    {
      switch (opt)
	{
	case 'r':
	  is_reverse = 1;
	  break;
	case '?':
	  print_help ();
	  return 0;
	case 'v':
	  print_version ();
	  return 0;
  case 'n':
    strcpy(socket_path, optarg);
    break;
	}
    }

  /** Create memif socket
   *
   * Interfaces are internally stored in a database referenced by memif socket.
   */
  /* Abstract socket supported */
  // memif_socket_args.path[0] = '@';
  // strncpy (memif_socket_args.path + 1, socket_path, strlen (socket_path));
  strncpy (memif_socket_args.path, socket_path, strlen (socket_path));
  /* Set application name */
  strncpy (memif_socket_args.app_name, socket_path, strlen (socket_path));

  err = memif_create_socket (&memif_socket, &memif_socket_args, NULL);
  if (err != MEMIF_ERR_SUCCESS)
    {
      INFO ("memif_create_socket: %s", memif_strerror (err));
      goto error;
    }

  /** Create memif interfaces
   *
   * Both interaces are assigned the same socket and same id to create a
   * loopback.
   */

  /* prepare the private data */
  memset (&intf0, 0, sizeof (intf0));
  memset (&intf1, 0, sizeof (intf1));
  if (is_reverse)
    {
      intf0.packet_handler = basic_packet_handler;
    }
  else
    {
      intf1.packet_handler = basic_packet_handler;
    }

  err =
    create_memif_interface (memif_socket, IF0_NAME, 0, /* master */ 1, &intf0);
  if (err != 0)
    {
      goto error;
    }

  // err =
  //   create_memif_interface (memif_socket, IF1_NAME, 0, /* slave */ 0, &intf1);
  // if (err != 0)
  //   {
  //     goto error;
  //   }

  do
    {
      err = memif_poll_event (memif_socket, -1);
    }
  while (err == MEMIF_ERR_SUCCESS);

  // remove socket file
  // Check if socket_path starts with '@'
  check_remove_file(0);

  return 0;

error:
  ret = -1;
done:
  free_memif_buffers (&intf0);
  free_memif_buffers (&intf1);
  memif_delete (&intf0.conn);
  memif_delete (&intf1.conn);
  memif_delete_socket (&memif_socket);
  return ret;
}
