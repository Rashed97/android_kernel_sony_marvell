#ifndef __I2C_BUS_H__
#define __I2C_BUS_H__

int galois_twsi_i2c_open(int master_id);
int galois_twsi_i2c_close(int master_id);
int galois_twsi_i2c_writeread(int master_id, int addr, int type,
		unsigned char *wbuf, int wlen, unsigned char *rbuf, int rlen);

#endif
