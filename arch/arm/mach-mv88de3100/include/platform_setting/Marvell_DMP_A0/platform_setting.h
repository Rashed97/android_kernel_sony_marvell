#define MV88DE3100_WIFI_WAKEUP_GPIO 16
#define MV88DE3100_BT_WAKEUP_GPIO 16

#define SDIO1_SET_VOLTAGE_3V3()			\
	do {					\
		GPIO_PortWrite(22, 1);		\
	} while (0)

#define SDIO1_SET_VOLTAGE_1V8()			\
	do {					\
		GPIO_PortWrite(22, 0);		\
	} while (0)

#define SDIO1_CLOSE_CARD_POWER()		\
	do {					\
		GPIO_PortWrite(7, 0);		\
	} while (0)

#define SDHC_INTERFACE0_FLAG	(MV_FLAG_CARD_PERMANENT	\
				| MV_FLAG_HOST_OFF_CARD_ON)
