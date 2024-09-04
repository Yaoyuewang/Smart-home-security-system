uint8_t wrprom(uint8_t *p, uint16_t n, uint16_t a);
uint8_t rdprom(uint8_t *p, uint16_t n, uint16_t a);
uint8_t i2c_io(uint8_t device_addr, uint8_t *wp, uint16_t wn, uint8_t *rp, uint16_t rn);
void i2c_init(uint8_t bdiv);
void sci_init(uint8_t ubrr);
void sci_out(char ch);
void sci_outs(char *s);