#include <common.h>
#include <assert.h>

int
responder (memif_conn_handle_t conn, void *private_ctx, uint16_t qid)
{
  memif_connection_t *c = (memif_connection_t *) private_ctx;
  int err, i;
  uint16_t tx;

  /* receive packets from the shared memory */
  err = memif_rx_burst (conn, qid, c->rx_bufs, MAX_MEMIF_BUFS, &c->rx_buf_num);
  if (err != MEMIF_ERR_SUCCESS)
    {
      INFO ("memif_rx_burst: %s", memif_strerror (err));
      return err;
    }

  do
    {
      /* allocate tx buffers */
      err = memif_buffer_alloc (conn, qid, c->tx_bufs, c->rx_buf_num,
				&c->tx_buf_num, c->buffer_size);
      /* suppress full ring error MEMIF_ERR_NOBUF_RING */
      if (err != MEMIF_ERR_SUCCESS && err != MEMIF_ERR_NOBUF_RING)
	{
	  INFO ("memif_buffer_alloc: %s", memif_strerror (err));
	  goto error;
	}

      /* Process the packets */
      if (c->packet_handler == NULL)
	{
	  INFO ("Missing packet handler");
	  goto error;
	}
      err = c->packet_handler (c);
      if (err != 0)
	{
	  INFO ("packet handler error: %d", err);
	  goto error;
	}
      /* Done processing packets */

      /* refill the queue */
      err = memif_refill_queue (conn, qid, c->tx_buf_num, c->headroom_size);
      if (err != MEMIF_ERR_SUCCESS)
	{
	  INFO ("memif_refill_queue: %s", memif_strerror (err));
	  goto error;
	}
      c->rx_buf_num -= c->tx_buf_num;

      err = memif_tx_burst (conn, qid, c->tx_bufs, c->tx_buf_num, &tx);
      if (err != MEMIF_ERR_SUCCESS)
	{
	  INFO ("memif_tx_burst: %s", memif_strerror (err));
	  goto error;
	}
      c->tx_buf_num -= tx;

      /* This should never happen */
      if (c->tx_buf_num != 0)
	{
	  INFO ("memif_tx_burst failed to send all allocated buffers.");
	  goto error;
	}
    }
  while (c->rx_buf_num > 0);

  return 0;

error:
  err = memif_refill_queue (conn, qid, c->rx_buf_num, c->headroom_size);
  if (err != MEMIF_ERR_SUCCESS)
    {
      INFO ("memif_refill_queue: %s", memif_strerror (err));
      return err;
    }
  c->rx_buf_num = 0;

  return -1;
}

int
responder_zero_copy (memif_conn_handle_t conn, void *private_ctx, uint16_t qid)
{
  memif_connection_t *c = (memif_connection_t *) private_ctx;
  int err, i;
  uint16_t tx, tx2;

  /* receive packets from the shared memory */
  err = memif_rx_burst (conn, qid, c->rx_bufs, MAX_MEMIF_BUFS, &c->rx_buf_num);
  if (err != MEMIF_ERR_SUCCESS)
    {
      INFO ("memif_rx_burst: %s", memif_strerror (err));
      return err;
    }

  do
    {
      /* Note that in zero copy memif_buffer_alloc is not part of respond
      process,
       * instead rx buffers are used directly using memif_buffer_enq_tx.
       * /

      /* Process the packets */
      if (c->packet_handler == NULL)
	{
	  INFO ("Missing packet handler");
	  goto error;
	}
      err = c->packet_handler (c);
      if (err != 0)
	{
	  INFO ("packet handler error: %d", err);
	  goto error;
	}
      /* Done processing packets */

      /* Swap rx and tx buffers, swapped tx buffers are considered allocated
       * and are ready to be transmitted. Notice that the buffers are swapped
       * only in memif driver and locally remain in rx_bufs queue.
       */
      err = memif_buffer_enq_tx (conn, qid, c->rx_bufs, c->rx_buf_num, &tx);
      /* suppress full ring error MEMIF_ERR_NOBUF_RING */
      if (err != MEMIF_ERR_SUCCESS && err != MEMIF_ERR_NOBUF_RING)
	{
	  INFO ("memif_buffer_alloc: %s", memif_strerror (err));
	  goto error;
	}

      /* refill the queue */
      err = memif_refill_queue (conn, qid, tx, c->headroom_size);
      if (err != MEMIF_ERR_SUCCESS)
	{
	  INFO ("memif_refill_queue: %s", memif_strerror (err));
	  goto error;
	}
      c->rx_buf_num -= tx;

      /* Notice that we send from rx_bufs as the buffers were only swapped
       * internally in memif driver */
      err = memif_tx_burst (conn, qid, c->rx_bufs, tx, &tx2);
      if (err != MEMIF_ERR_SUCCESS)
	{
	  INFO ("memif_tx_burst: %s", memif_strerror (err));
	  goto error;
	}
      tx -= tx2;

      /* This should never happen */
      if (tx != 0)
	{
	  INFO ("memif_tx_burst failed to send all allocated buffers.");
	  goto error;
	}
    }
  while (c->rx_buf_num > 0);

  return 0;

error:
  err = memif_refill_queue (conn, qid, c->rx_buf_num, c->headroom_size);
  if (err != MEMIF_ERR_SUCCESS)
    {
      INFO ("memif_refill_queue: %s", memif_strerror (err));
      return err;
    }
  c->rx_buf_num = 0;

  return -1;
}

void process_data_memset( memif_connection_t *c, int flag, int need_size){
    memif_buffer_t *mb;
    for (int i = 0; i < c->tx_buf_num; i++)
    {
      // memcpy (c->tx_bufs[i].data, c->rx_bufs[i].data, c->rx_bufs[i].len);
      	mb = &c->tx_bufs[i];
        assert(mb->len == c->buffer_size);
        int *a = (int *)mb->data;
        // for (int j = 0; j < mb->len / sizeof(int); j++)
	      //   a[j] = flag;
        memset(a, 0, mb->len);
        a[0] = flag;
        a[flag % (need_size / 4)] = flag;
    }
}
int B[1024] = {1,2,3,4,5};

void process_data( memif_connection_t *c, int flag, int need_size){
    memif_buffer_t *mb;
    for (int i = 0; i < c->tx_buf_num; i++)
    {
      // memcpy (c->tx_bufs[i].data, c->rx_bufs[i].data, c->rx_bufs[i].len);
      	mb = &c->tx_bufs[i];
        assert(mb->len == c->buffer_size);
        int *a = (int *)mb->data;
        // for (int j = 0; j < mb->len / sizeof(int); j++)
	      //   a[j] = flag;
        // memset(a, 0, mb->len);
        memcpy(a, B, need_size);
        a[0] = flag;
        a[flag % (need_size / 4)] = flag;
    }
}

void process_data_all( memif_connection_t *c, int flag, int need_size){
    memif_buffer_t *mb;
    for (int i = 0; i < c->tx_buf_num; i++)
    {
      // memcpy (c->tx_bufs[i].data, c->rx_bufs[i].data, c->rx_bufs[i].len);
      	mb = &c->tx_bufs[i];
        assert(mb->len == c->buffer_size);
        int *a = (int *)mb->data;
        for (int j = 0; j < mb->len / sizeof(int); j++)
	        a[j] = flag;
    }
}

int
responder_bandwidth (memif_conn_handle_t conn, void *private_ctx, uint16_t qid)
{
  // INFO("responder_bandwidth c->tx_buf_num=%d", c->tx_buf_num);
  memif_connection_t *c = (memif_connection_t *) private_ctx;
  int err, i;
  uint16_t tx;
  int need_send = MAX_MEMIF_BUFS, need_size = 1024;
  c->buffer_size = need_size;

  /* receive packets from the shared memory */
  // err = memif_rx_burst (conn, qid, c->rx_bufs, MAX_MEMIF_BUFS, &c->rx_buf_num);
  err = memif_rx_burst (conn, qid, c->rx_bufs, 1, &c->rx_buf_num);
  if (err != MEMIF_ERR_SUCCESS)
    {
      INFO ("memif_rx_burst: %s", memif_strerror (err));
      return err;
    }
  assert(c->rx_buf_num == 1);
  char *ch = (char *)(c->rx_bufs[0].data);
  assert(ch[0] == 'O' && ch[1] == 'K');
  int flag = ((int *)(ch + 2))[0];
  // INFO("[responder] flag:%d", flag);
  // INFO("[before] c->tx_bufs:%p", c->tx_bufs);
  do
    {
      if (1 || c->tx_buf_num != need_send){
        // INFO("in alloc [respond] c->tx_buf_num:%d", c->tx_buf_num);
      /* allocate tx buffers */
      err = memif_buffer_alloc (conn, qid, c->tx_bufs, 
        // c->rx_buf_num,
        need_send, // send max size
				&c->tx_buf_num, 
        c->buffer_size);
      /* suppress full ring error MEMIF_ERR_NOBUF_RING */
      if (err != MEMIF_ERR_SUCCESS && err != MEMIF_ERR_NOBUF_RING)
	{
	  INFO ("memif_buffer_alloc: %s", memif_strerror (err));
	  goto error;
	}
  assert(c->tx_buf_num == need_send);
      }
  // INFO("[after] c->tx_bufs:%p", c->tx_bufs);
  // INFO("worker respond alloc buf_num:%d c->buffer_size:%d", c->tx_buf_num, c->buffer_size);

      /* Process the packets */
  //     if (c->packet_handler == NULL)
	// {
	//   INFO ("Missing packet handler");
	//   goto error;
	// }
  //     err = c->packet_handler (c);
  //     if (err != 0)
	// {
	//   INFO ("packet handler error: %d", err);
	//   goto error;
	// }

    // process_data_memset(c, flag, need_size);
    // process_data_all(c, flag, need_size);
    process_data(c, flag, need_size);




      /* Done processing packets */

      /* refill the queue */
      err = memif_refill_queue (conn, qid, c->tx_buf_num, c->headroom_size);
      if (err != MEMIF_ERR_SUCCESS)
	{
	  INFO ("memif_refill_queue: %s", memif_strerror (err));
	  goto error;
	}
      // c->rx_buf_num -= c->tx_buf_num;

      err = memif_tx_burst (conn, qid, c->tx_bufs, c->tx_buf_num, &tx);
      if (err != MEMIF_ERR_SUCCESS)
	{
	  INFO ("memif_tx_burst: %s", memif_strerror (err));
	  goto error;
	}
      c->tx_buf_num -= tx;

      /* This should never happen */
      if (c->tx_buf_num != 0)
	{
	  INFO ("memif_tx_burst failed to send all allocated buffers.");
	  goto error;
	}
  // INFO("END tx_buf_num:%d", c->tx_buf_num);
    }
  // while (c->rx_buf_num > 0);
  while (c->tx_buf_num > 0);
  c->rx_buf_num = 0;
  c->tx_buf_num = need_send;

  return 0;

error:
  err = memif_refill_queue (conn, qid, c->rx_buf_num, c->headroom_size);
  if (err != MEMIF_ERR_SUCCESS)
    {
      INFO ("memif_refill_queue: %s", memif_strerror (err));
      return err;
    }
  c->rx_buf_num = 0;

  return -1;
}