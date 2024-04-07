#include <common.h>
#include <assert.h>
int
send_packets (memif_connection_t *c, uint16_t qid,
	      packet_generator_t *generator, uint32_t num_pkts,
	      uint16_t max_pkt_size)
{
  int err, i;
  uint16_t tx;

  do
    {
      err = memif_buffer_alloc (c->conn, qid, c->tx_bufs,
				num_pkts > MAX_MEMIF_BUFS ? MAX_MEMIF_BUFS :
								  num_pkts,
				&c->tx_buf_num, max_pkt_size);
      /* suppress full ring error MEMIF_ERR_NOBUF_RING */
      if (err != MEMIF_ERR_SUCCESS && err != MEMIF_ERR_NOBUF_RING)
	{
	  INFO ("memif_buffer_alloc: %s", memif_strerror (err));
	  goto error;
	}

      /* generate packet inside allocated buffers */
      err = generator (c, num_pkts);
      if (err != 0)
	{
	  INFO ("paclet generator error: %d", err);
	  goto error;
	}

      err = memif_tx_burst (c->conn, qid, c->tx_bufs, c->tx_buf_num, &tx);
      if (err != MEMIF_ERR_SUCCESS)
	{
	  INFO ("memif_tx_burst: %s", memif_strerror (err));
	  goto error;
	}
      c->tx_buf_num -= tx;

      /* Should never happen... */
      if (c->tx_buf_num > 0)
	{
	  INFO ("Failed to send allocated packets");
	  goto error;
	}
      num_pkts -= tx;
    }
  while (num_pkts > 0);

  return 0;

error:
  /* TODO: free alloocated tx buffers */
  return -1;
}

int
send_packets2 (memif_connection_t *c, uint16_t qid,
	       packet_generator_t *generator, uint32_t num_pkts,
	       uint16_t max_pkt_size, int flag)
{
  int err, i;
  uint16_t tx;
  // INFO("sned_packets %d %d", num_pkts, max_pkt_size);
  do
    {
      err = memif_buffer_alloc (c->conn, qid, c->tx_bufs,
				num_pkts > MAX_MEMIF_BUFS ? MAX_MEMIF_BUFS :
							    num_pkts,
				&c->tx_buf_num, max_pkt_size);
      /* suppress full ring error MEMIF_ERR_NOBUF_RING */
      if (err != MEMIF_ERR_SUCCESS && err != MEMIF_ERR_NOBUF_RING)
	{
	  INFO ("memif_buffer_alloc: %s", memif_strerror (err));
	  goto error;
	}
      // INFO("sender num_pkts=%d tx_buf_num=%d len0=%d",num_pkts,
      // c->tx_buf_num, c->tx_bufs[0].len);

      /* generate packet inside allocated buffers */
      //     err = generator (c, num_pkts);
      //     if (err != 0)
      // {
      //   INFO ("paclet generator error: %d", err);
      //   goto error;
      // }

      int bi = 0;
      memif_buffer_t *mb;
      if (flag < 0) // OK
	{
	  for (int i = 0; (i < num_pkts) && (bi < c->tx_buf_num); i++)
	    {
	      mb = &c->tx_bufs[bi++];
        assert(mb->len == max_pkt_size);
        char *b = (char *)mb->data;
        b[0] = 'O';
        b[1] = 'K';
        int *a = (int *)(b+2);
        a[0] = -flag;
        // for (int j = 0; j < max_pkt_size / sizeof(int); j++)
	      //   a[j] = -flag;
	    }  
	}
  else if (flag > 0){
	  for (int i = 0; (i < num_pkts) && (bi < c->tx_buf_num); i++)
	    {
	      mb = &c->tx_bufs[bi++];
        assert(mb->len == max_pkt_size);
        int *a = (int *)mb->data;
        for (int j = 0; j < max_pkt_size / sizeof(int); j++)
	        a[j] = flag;
	    }    
  }

      err = memif_tx_burst (c->conn, qid, c->tx_bufs, c->tx_buf_num, &tx);
      if (err != MEMIF_ERR_SUCCESS)
	{
	  INFO ("memif_tx_burst: %s", memif_strerror (err));
	  goto error;
	}
      c->tx_buf_num -= tx;

      /* Should never happen... */
      if (c->tx_buf_num > 0)
	{
	  INFO ("Failed to send allocated packets");
	  goto error;
	}
      num_pkts -= tx;
    }
  while (num_pkts > 0);

  return 0;

error:
  /* TODO: free alloocated tx buffers */
  return -1;
}