// TC74A5 (check address in datasheet! is on SoftSDA and SoftSCL with 10k pull up resistors)
startDriver I2C
setChannelType 6 temperature
// Options are: I2C1 (BK port), I2C2 (BK port), Soft (pins set in configure module)
// 0x4D is device address 
// 6 is target channel
addI2CDevice_TC74 Soft 0x4D 6