// https://github.com/lNikazzzl/tcl_ac_esphome/tree/master

typedef union get_cmd_resp_s {
	struct {
		uint8_t header;
		uint8_t byte_1;
		uint8_t byte_2;
		uint8_t type;
		uint8_t len;
		uint8_t byte_5;
		uint8_t byte_6;

		uint8_t mode : 4;
		uint8_t power : 1;
		uint8_t disp : 1;
		uint8_t eco : 1;
		uint8_t turbo : 1;

		uint8_t temp : 4;
		uint8_t fan : 3;
		uint8_t byte_8_bit_7 : 1;

		uint8_t byte_9;

		uint8_t byte_10_bit_0_4 : 5;
		uint8_t hswing : 1;
		uint8_t vswing : 1;
		uint8_t byte_10_bit_7 : 1;

		uint8_t byte_11;
		uint8_t byte_12;
		uint8_t byte_13;
		uint8_t byte_14;
		uint8_t byte_15;
		uint8_t byte_16;
		uint8_t byte_17;
		uint8_t byte_18;
		uint8_t byte_19;

		uint8_t byte_20;
		uint8_t byte_21;
		uint8_t byte_22;
		uint8_t byte_23;
		uint8_t byte_24;
		uint8_t byte_25;
		uint8_t byte_26;
		uint8_t byte_27;
		uint8_t byte_28;
		uint8_t byte_29;

		uint8_t byte_30;
		uint8_t byte_31;
		uint8_t byte_32;

		uint8_t byte_33_bit_0_6 : 7;
		uint8_t mute : 1;

		uint8_t byte_34;
		uint8_t byte_35;
		uint8_t byte_36;
		uint8_t byte_37;
		uint8_t byte_38;
		uint8_t byte_39;

		uint8_t byte_40;
		uint8_t byte_41;
		uint8_t byte_42;
		uint8_t byte_43;
		uint8_t byte_44;
		uint8_t byte_45;
		uint8_t byte_46;
		uint8_t byte_47;
		uint8_t byte_48;
		uint8_t byte_49;

		uint8_t byte_50;

		uint8_t vswing_fix : 3;
		uint8_t vswing_mv : 2;
		uint8_t byte_51_bit_5_7 : 3;

		uint8_t hswing_fix : 3;
		uint8_t hswing_mv : 3;
		uint8_t byte_52_bit_6_7 : 2;

		uint8_t byte_53;
		uint8_t byte_54;
		uint8_t byte_55;
		uint8_t byte_56;
		uint8_t byte_57;
		uint8_t byte_58;
		uint8_t byte_59;

		uint8_t xor_sum;
	} data;
	uint8_t raw[61];
} get_cmd_resp_t;

typedef union set_cmd_u {
	struct {
		uint8_t header;
		uint8_t byte_1;
		uint8_t byte_2;
		uint8_t type;
		uint8_t len;
		uint8_t byte_5;
		uint8_t byte_6;

		uint8_t byte_7_bit_0_1 : 2;
		uint8_t power : 1;
		uint8_t off_timer_en : 1;
		uint8_t on_timer_en : 1;
		uint8_t beep : 1;
		uint8_t disp : 1;
		uint8_t eco : 1;

		uint8_t mode : 4;
		uint8_t byte_8_bit_4_5 : 2;
		uint8_t turbo : 1;
		uint8_t mute : 1;

		uint8_t temp : 4;
		uint8_t byte_9_bit_4_7 : 4;

		uint8_t fan : 3;
		uint8_t vswing : 3;
		uint8_t byte_10_bit_6 : 1;
		uint8_t byte_10_bit_7 : 1;

		uint8_t byte_11_bit_0_2 : 3;
		uint8_t hswing : 1;
		uint8_t byte_11_bit_4_7 : 4;

		uint8_t byte_12;
		uint8_t byte_13;

		uint8_t byte_14_bit_0_2 : 3;
		uint8_t byte_14_bit_3 : 1;
		uint8_t byte_14_bit_4 : 1;//uint8_t hswing : 1;
		uint8_t half_degree : 1;
		uint8_t byte_14_bit_6_7 : 2;

		uint8_t byte_15;
		uint8_t byte_16;
		uint8_t byte_17;
		uint8_t byte_18;
		uint8_t byte_19;

		uint8_t byte_20;
		uint8_t byte_21;
		uint8_t byte_22;
		uint8_t byte_23;
		uint8_t byte_24;
		uint8_t byte_25;
		uint8_t byte_26;
		uint8_t byte_27;
		uint8_t byte_28;
		uint8_t byte_29;

		uint8_t byte_30;
		uint8_t byte_31;
		uint8_t vswing_fix : 3;
		uint8_t vswing_mv : 2;
		uint8_t byte_32_bit_5_7 : 3;

		uint8_t hswing_fix : 3;
		uint8_t hswing_mv : 3;
		uint8_t byte_33_bit_6_7 : 2;

		uint8_t xor_sum;
	} data;
	uint8_t raw[35];
} set_cmd_t;