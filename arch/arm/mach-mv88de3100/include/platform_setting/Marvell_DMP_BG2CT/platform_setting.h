#define SDIO0_SET_VOLTAGE_3V3()			\
	do {					\
		GPIO_PortWrite(21, 1);		\
	} while (0)

#define SDIO0_SET_VOLTAGE_1V8()			\
	do {					\
		GPIO_PortWrite(21, 0);		\
	} while (0)

#define SDIO1_SET_VOLTAGE_3V3()			\
	do {					\
		GPIO_PortWrite(22, 1);		\
	} while (0)

#define SDIO1_SET_VOLTAGE_1V8()			\
	do {					\
		GPIO_PortWrite(22, 0);		\
	} while (0)

#define SDHC_INTERFACE0_FLAG	(MV_FLAG_CARD_PERMANENT	\
				| MV_FLAG_HOST_OFF_CARD_ON)
