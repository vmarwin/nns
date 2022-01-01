/* bot.cpp        */
/* version 1.10   */
/* mikh.vl@gmail.com   */

char version[] = { "1.10" };

#include "define.h"
#include "data.h"
#include "trade.h"
#include "fnn.h"

volatile bool is_thread_stop = false;

char isin[CSIZE] = {};
char cid[] = { };
char broker_code[] = { };
char client_code[] = { };
double ask = 0.0;
double bid = 0.0;
uint64_t sec = 0;
bool fut_is_ready = false;
int fut_isin_id = 0;
int matching_id = 1;
bool mid_is_ready = false;
cg_time_t server_time{0};
long long xpos = 0;
int session_state = 0;
int code = 0;
char message[200];
long long order_id = 0;
bool hb_is_ready = false;
bool bcc_is_ready = false;
bool bcid_is_ready = false;
char bcc[CSIZE];
int bcid = 0;
double money_amount = 0.0;
double money_blocked = 0.0;
double money_free = 0.0;
double fee = 0.0;
int session_id = 0;
bool mid_flag = false;
double buy_deposit = 0.0 ;
double sell_deposit = 0.0;
int digits = 0;
double step_size = 0.0;
double step_cost = 0.0;
bool code_332 = false;
char price_format[CSIZE] = { "" };
bool pdev_flag = false;
double pdev = 20;

void InterruptHandler(int signal)
{
	is_thread_stop = true;
}
CG_RESULT MessageCallbackFut(cg_conn_t* conn, cg_listener_t* listener, struct cg_msg_t* msg, void* data)
{
	static size_t offset_isin = 0;
	static size_t offset_isin_id = 0;
	static size_t offset_state = 0;
	static size_t offset_sess_id = 0;
	static size_t offset_base_contract_code = 0; //c25
	static size_t offset_replAct = 0; //i8
	static size_t offset_buy_deposit = 0; //d16.2
	static size_t offset_sell_deposit = 0; //d16.2
	static size_t offset_roundto = 0;//i4
	static size_t offset_min_step = 0;//d16.5
	static size_t offset_step_price = 0; //d16.5

	switch (msg->type)
	{
	case CG_MSG_STREAM_DATA:
	{
		int64_t price_int = 0;
		signed char price_scale = 0;

		cg_msg_streamdata_t* replmsg = (cg_msg_streamdata_t*)msg;
		char* msg_data = (char*)replmsg->data;
		if (strcmp(((char*)(msg_data + offset_isin)), isin) == 0 && *(long long*)(msg_data + offset_replAct) == 0)
		{
			fut_isin_id = *(int*)(msg_data + offset_isin_id);

			strcpy(bcc, ((char*)(msg_data + offset_base_contract_code)));
			bcc_is_ready = true;

			session_id = *((int*)(msg_data + offset_sess_id));
			session_state = *((int*)(msg_data + offset_state));

			cg_bcd_get(((char*)(msg_data + offset_buy_deposit)), &price_int, &price_scale);
			buy_deposit = ((double)price_int) / (pow(10.0, price_scale));

			cg_bcd_get(((char*)(msg_data + offset_sell_deposit)), &price_int, &price_scale);
			sell_deposit = ((double)price_int) / (pow(10.0, price_scale));

			digits = *(int*)(msg_data + offset_roundto);
				
			cg_bcd_get(((char*)(msg_data + offset_min_step)), &price_int, &price_scale);
			step_size = ((double)price_int) / (pow(10.0, price_scale));
			
			cg_bcd_get(((char*)(msg_data + offset_step_price)), &price_int, &price_scale);
			step_cost = ((double)price_int) / (pow(10.0, price_scale));

			if (!pdev_flag)
			{
				pdev *= step_size;
				pdev_flag = true;
			}
			
			sprintf(price_format, "%%.%d%s", digits, "f");

			printf("%s: Session: %d, state: %d, isin: %s, isin id: %d, buy: %G, sell: %G; digits: %d; size: %G; cost: %G; pd: %G\n",
				g_cserver_time, session_id, session_state, isin, fut_isin_id, buy_deposit, sell_deposit, digits, step_size, step_cost, pdev);
		}
	}
	break;

	case CG_MSG_P2REPL_ONLINE:
	{
		printf("%s: Futures Reference online!\n", g_cserver_time);
	}
	break;

	case CG_MSG_CLOSE:
	{
		printf("%s: Futures Reference offline!\n", g_cserver_time);
	}
	break;

	case CG_MSG_OPEN:
	{
		struct cg_scheme_desc_t* schemedesc = 0;
		cg_lsn_getscheme(listener, &schemedesc);

		size_t msgidx = 0;
		for (cg_message_desc_t* msgdesc = schemedesc->messages; msgdesc; msgdesc = msgdesc->next, msgidx++)
		{
			size_t fieldindex = 0;
			if (strcmp(msgdesc->name, "fut_sess_contents") == 0)
			{
				for (cg_field_desc_t* fielddesc = msgdesc->fields; fielddesc; fielddesc = fielddesc->next, fieldindex++)
				{
					if (strcmp(fielddesc->name, "replAct") == 0 && strcmp(fielddesc->type, "i8") == 0)
					{
						offset_replAct = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "isin_id") == 0 && strcmp(fielddesc->type, "i4") == 0)
					{
						offset_isin_id = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "isin") == 0 && strcmp(fielddesc->type, "c25") == 0)
					{
						offset_isin = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "base_contract_code") == 0 && strcmp(fielddesc->type, "c25") == 0)
					{
						offset_base_contract_code = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "sess_id") == 0 && strcmp(fielddesc->type, "i4") == 0)
					{
						offset_sess_id = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "state") == 0 && strcmp(fielddesc->type, "i4") == 0)
					{
						offset_state = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "buy_deposit") == 0 && strcmp(fielddesc->type, "d16.2") == 0)
					{
						offset_buy_deposit = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "sell_deposit") == 0 && strcmp(fielddesc->type, "d16.2") == 0)
					{
						offset_sell_deposit = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "roundto") == 0 && strcmp(fielddesc->type, "i4") == 0)
					{
						offset_roundto = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "min_step") == 0 && strcmp(fielddesc->type, "d16.5") == 0)
					{
						offset_min_step = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "step_price") == 0 && strcmp(fielddesc->type, "d16.5") == 0)
					{
						offset_step_price = fielddesc->offset;
					}
				}
			}
		}
	}
	break;
	}

	return 0;
}
CG_RESULT MessageCallbackHB(cg_conn_t* conn, cg_listener_t* listener, struct cg_msg_t* msg, void* data)
{
	static size_t offset_server_time{ 0 }; //t

	switch (msg->type)
	{
	case CG_MSG_STREAM_DATA:
	{
		cg_msg_streamdata_t* replmsg = (cg_msg_streamdata_t*)msg;
		char* msg_data = (char*)replmsg->data;

		server_time = *((cg_time_t*)(msg_data + offset_server_time));
		sprintf(g_cserver_time, "%04d.%02d.%02d %02d:%02d:%02d",
			server_time.year, server_time.month, server_time.day, server_time.hour,
			server_time.minute, server_time.second);
	}
	break;

	case CG_MSG_P2REPL_ONLINE:
	{
		printf("%s: Heartbeat online!\n", g_cserver_time);
		hb_is_ready = true;
	}
	break;

	case CG_MSG_CLOSE:
	{
		printf("%s: Heartbeat offline!\n", g_cserver_time);
		hb_is_ready = false;
	}
	break;

	case CG_MSG_OPEN:
	{
		struct cg_scheme_desc_t* schemedesc = 0;
		cg_lsn_getscheme(listener, &schemedesc);

		size_t msgidx = 0;
		for (cg_message_desc_t* msgdesc = schemedesc->messages; msgdesc; msgdesc = msgdesc->next, msgidx++)
		{
			size_t fieldindex = 0;
			if (strcmp(msgdesc->name, "heartbeat") == 0)
			{
				for (cg_field_desc_t* fielddesc = msgdesc->fields; fielddesc; fielddesc = fielddesc->next, fieldindex++)
				{
					if (strcmp(fielddesc->name, "server_time") == 0 && strcmp(fielddesc->type, "t") == 0)
					{
						offset_server_time = fielddesc->offset;
					}
				}
			}
		}
	}
	break;
	}

	return 0;
}
CG_RESULT MessageCallbackBCID(cg_conn_t* conn, cg_listener_t* listener, struct cg_msg_t* msg, void* data)
{
	static size_t offset_base_contract_code_vcb = 0; //c25
	static size_t offset_base_contract_id = 0; //i4

	switch (msg->type)
	{
	case CG_MSG_STREAM_DATA:
	{
		cg_msg_streamdata_t* replmsg = (cg_msg_streamdata_t*)msg;
		char* msg_data = (char*)replmsg->data;
		if (strcmp(bcc, ((char*)(msg_data + offset_base_contract_code_vcb))) == 0)
		{
			bcid = *(int*)(msg_data + offset_base_contract_id);
			bcid_is_ready = true;
		}
	}
	break;

	case CG_MSG_P2REPL_ONLINE:
	{
		printf("%s: BCID online!\n", g_cserver_time);
	}
	break;

	case CG_MSG_CLOSE:
	{
		printf("%s: BCID offline!\n", g_cserver_time);
	}
	break;

	case CG_MSG_OPEN:
	{
		struct cg_scheme_desc_t* schemedesc = 0;
		cg_lsn_getscheme(listener, &schemedesc);

		size_t msgidx = 0;
		for (cg_message_desc_t* msgdesc = schemedesc->messages; msgdesc; msgdesc = msgdesc->next, msgidx++)
		{
			size_t fieldindex = 0;
			if (strcmp(msgdesc->name, "fut_vcb") == 0)
			{
				for (cg_field_desc_t* fielddesc = msgdesc->fields; fielddesc; fielddesc = fielddesc->next, fieldindex++)
				{
					if (strcmp(fielddesc->name, "base_contract_code") == 0 && strcmp(fielddesc->type, "c25") == 0)
					{
						offset_base_contract_code_vcb = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "base_contract_id") == 0 && strcmp(fielddesc->type, "i4") == 0)
					{
						offset_base_contract_id = fielddesc->offset;
					}
				}
			}
		}
	}
	break;
	}

	return 0;
}
CG_RESULT MessageCallbackMID(cg_conn_t* conn, cg_listener_t* listener, struct cg_msg_t* msg, void* data)
{
	static size_t offset_base_contract_id = 0; //i4
	static size_t offset_matching_id = 0; //i1

	switch (msg->type)
	{
	case CG_MSG_STREAM_DATA:
	{
		cg_msg_streamdata_t* replmsg = (cg_msg_streamdata_t*)msg;
		char* msg_data = (char*)replmsg->data;

		if (bcid == *((int*)(msg_data + offset_base_contract_id)))
		{
			matching_id = *(int8_t*)(msg_data + offset_matching_id);
			mid_is_ready = true;
		}
	}
	break;

	case CG_MSG_P2REPL_ONLINE:
	{
		printf("%s: MID online!\n", g_cserver_time);
		mid_flag = true;
	}
	break;

	case CG_MSG_CLOSE:
	{
		printf("%s: MID offline!\n", g_cserver_time);
		mid_flag = false;
	}
	break;

	case CG_MSG_OPEN:
	{
		struct cg_scheme_desc_t* schemedesc = 0;
		cg_lsn_getscheme(listener, &schemedesc);

		size_t msgidx = 0;
		for (cg_message_desc_t* msgdesc = schemedesc->messages; msgdesc; msgdesc = msgdesc->next, msgidx++)
		{
			size_t fieldindex = 0;
			if (strcmp(msgdesc->name, "instr2matching_map") == 0)
			{
				for (cg_field_desc_t* fielddesc = msgdesc->fields; fielddesc; fielddesc = fielddesc->next, fieldindex++)
				{
					if (strcmp(fielddesc->name, "base_contract_id") == 0 && strcmp(fielddesc->type, "i4") == 0)
					{
						offset_base_contract_id = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "matching_id") == 0 && strcmp(fielddesc->type, "i1") == 0)
					{
						offset_matching_id = fielddesc->offset;
					}
				}
			}
		}
	}
	break;
	}

	return 0;
}
CG_RESULT MessageCallbackPart(cg_conn_t* conn, cg_listener_t* listener, struct cg_msg_t* msg, void* data)
{
	static size_t offset_money_amount = 0;
	static size_t offset_money_free = 0;
	static size_t offset_money_blocked = 0;
	static size_t offset_fee = 0;
	static size_t offset_client_code = 0; //c7

	switch (msg->type)
	{
	case CG_MSG_STREAM_DATA:
	{
		int64_t price_int = 0;
		signed char price_scale = 0;
		char client_code[80];

		cg_msg_streamdata_t* replmsg = (cg_msg_streamdata_t*)msg;

		char* msg_data = (char*)replmsg->data;

		cg_bcd_get(((char*)(msg_data + offset_money_amount)), &price_int, &price_scale);
		money_amount = ((double)price_int) / (pow(10.0, price_scale));

		cg_bcd_get(((char*)(msg_data + offset_money_free)), &price_int, &price_scale);
		money_free = ((double)price_int) / (pow(10.0, price_scale));

		cg_bcd_get(((char*)(msg_data + offset_money_blocked)), &price_int, &price_scale);
		money_blocked = ((double)price_int) / (pow(10.0, price_scale));

		cg_bcd_get(((char*)(msg_data + offset_fee)), &price_int, &price_scale);
		fee = ((double)price_int) / (pow(10.0, price_scale));

		strcpy(client_code, ((char*)(msg_data + offset_client_code)));

		if (strcmp(client_code, cid) != 0)
		{
			//printf("%s: Wrong client code! Bot shutdown!\n", g_cserver_time);
			is_thread_stop = true;
		}

		printf("%s: Client: %s; Amount: %G; Free: %G; Blocked: %G; Fee: %G\n", g_cserver_time,
			client_code, money_amount, money_free, money_blocked, fee);
	}
	break;

	case CG_MSG_P2REPL_ONLINE:
	{
		printf("%s: Account online!\n", g_cserver_time);
	}
	break;

	case CG_MSG_CLOSE:
	{
		printf("%s: Account offline!\n", g_cserver_time);
	}
	break;

	case CG_MSG_OPEN:
	{
		struct cg_scheme_desc_t* schemedesc = 0;
		cg_lsn_getscheme(listener, &schemedesc);
		size_t msgidx = 0;
		for (cg_message_desc_t* msgdesc = schemedesc->messages; msgdesc; msgdesc = msgdesc->next, msgidx++)
		{
			size_t fieldindex = 0;
			if (strcmp(msgdesc->name, "part") == 0)
			{
				for (cg_field_desc_t* fielddesc = msgdesc->fields; fielddesc; fielddesc = fielddesc->next, fieldindex++)
				{
					if (strcmp(fielddesc->name, "money_free") == 0 && strcmp(fielddesc->type, "d26.2") == 0)
					{
						offset_money_free = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "money_blocked") == 0 && strcmp(fielddesc->type, "d26.2") == 0)
					{
						offset_money_blocked = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "fee") == 0 && strcmp(fielddesc->type, "d26.2") == 0)
					{
						offset_fee = fielddesc->offset;
					}
					if (strcmp(fielddesc->name, "money_amount") == 0 && strcmp(fielddesc->type, "d26.2") == 0)
					{
						offset_money_amount = fielddesc->offset;
					}
					if (strcmp(fielddesc->name, "client_code") == 0 && strcmp(fielddesc->type, "c7") == 0)
					{
						offset_client_code = fielddesc->offset;
					}
				}
			}
		}
	}
	break;
	}

	return 0;
}
CG_RESULT MessageCallbackPos(cg_conn_t* conn, cg_listener_t* listener, struct cg_msg_t* msg, void* data)
{
	static size_t offset_isin_id = 0; // i4
	static size_t offset_xpos = 0; // i8

	switch (msg->type)
	{

	case CG_MSG_STREAM_DATA:
	{
		cg_msg_streamdata_t* replmsg = (cg_msg_streamdata_t*)msg;
		char* msg_data = (char*)replmsg->data;

		if (fut_isin_id == *((int*)(msg_data + offset_isin_id)))
		{
			xpos = *((long long*)(msg_data + offset_xpos));

			cg_log_info("------- GET POSITION %s; XPOS: %lld -------", xpos > 0 ? "BUY" : "SELL", xpos);
		}
	}
	break;

	case CG_MSG_P2REPL_ONLINE:
	{
		printf("%s: Position online!\n", g_cserver_time);
	}
	break;

	case CG_MSG_CLOSE:
	{
		printf("%s: Position offline!\n", g_cserver_time);
	}
	break;

	case CG_MSG_OPEN:
	{
		struct cg_scheme_desc_t* schemedesc = 0;
		cg_lsn_getscheme(listener, &schemedesc);

		size_t msgidx = 0;
		for (cg_message_desc_t* msgdesc = schemedesc->messages; msgdesc; msgdesc = msgdesc->next, msgidx++)
		{
			size_t fieldindex = 0;
			if (strcmp(msgdesc->name, "position") == 0)
			{
				for (cg_field_desc_t* fielddesc = msgdesc->fields; fielddesc; fielddesc = fielddesc->next, fieldindex++)
				{
					if (strcmp(fielddesc->name, "isin_id") == 0 && strcmp(fielddesc->type, "i4") == 0)
					{
						offset_isin_id = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "xpos") == 0 && strcmp(fielddesc->type, "i8") == 0)
					{
						offset_xpos = fielddesc->offset;
					}
				}
			}
		}
	}
	break;
	}

	return 0;
}
CG_RESULT MessageCallbackQuote(cg_conn_t* conn, cg_listener_t* listener, struct cg_msg_t* msg, void* data)
{
	static size_t offset_isin_id = 0;
	static size_t offset_best_buy = 0;
	static size_t offset_best_sell = 0;
	static size_t offset_replAct = 0; //i8
	static size_t offset_mod_time_ns = 0; //u8

	switch (msg->type)
	{
	case CG_MSG_STREAM_DATA:
	{
		int64_t price_int = 0;
		signed char price_scale = 0;

		cg_msg_streamdata_t* replmsg = (cg_msg_streamdata_t*)msg;
		char* msg_data = (char*)replmsg->data;

		if (*(long long*)(msg_data + offset_replAct) == 0 && fut_isin_id == *((int*)(msg_data + offset_isin_id)))
		{
			cg_bcd_get(((char*)(msg_data + offset_best_buy)), &price_int, &price_scale);
			bid = ((double)price_int) / (pow(10.0, price_scale));

			cg_bcd_get(((char*)(msg_data + offset_best_sell)), &price_int, &price_scale);
			ask = ((double)price_int) / (pow(10.0, price_scale));

			sec = (*(uint64_t*)(msg_data + offset_mod_time_ns)) / 1000000000;
		}
	}
	break;
	case CG_MSG_OPEN:
	{
		struct cg_scheme_desc_t* schemedesc = 0;
		cg_lsn_getscheme(listener, &schemedesc);

		size_t msgidx = 0;
		for (cg_message_desc_t* msgdesc = schemedesc->messages; msgdesc; msgdesc = msgdesc->next, msgidx++)
		{
			size_t fieldindex = 0;
			if (strcmp(msgdesc->name, "common") == 0)
			{
				for (cg_field_desc_t* fielddesc = msgdesc->fields; fielddesc; fielddesc = fielddesc->next, fieldindex++)
				{
					if (strcmp(fielddesc->name, "replAct") == 0 && strcmp(fielddesc->type, "i8") == 0)
					{
						offset_replAct = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "isin_id") == 0 && strcmp(fielddesc->type, "i4") == 0)
					{
						offset_isin_id = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "best_buy") == 0 && strcmp(fielddesc->type, "d16.5") == 0)
					{
						offset_best_buy = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "best_sell") == 0 && strcmp(fielddesc->type, "d16.5") == 0)
					{
						offset_best_sell = fielddesc->offset;
					}
					else if (strcmp(fielddesc->name, "mod_time_ns") == 0 && strcmp(fielddesc->type, "u8") == 0)
					{
						offset_mod_time_ns = fielddesc->offset;
					}
				}
			}
		}
	}
	break;
	case CG_MSG_P2REPL_ONLINE:
	{
		printf("%s: Futures quote online!\n", g_cserver_time);
		fut_is_ready = true;
	}
	break;
	case CG_MSG_CLOSE:
	{
		printf("%s: Futures quote offline!\n", g_cserver_time);
		fut_is_ready = false;
	}
	break;
	}
	return 0;
};
CG_RESULT MessageCallbackMSG(cg_conn_t* conn, cg_listener_t* listener, struct cg_msg_t* msg, void* data)
{
	switch (msg->type)
	{
	case CG_MSG_DATA:
	{
		struct cg_scheme_desc_t* scheme;
		size_t index_179 = 0;
		size_t offset_179_code = 0;
		size_t offset_179_message = 0;
		size_t offset_179_order_id = 0;

		size_t index_186 = 0;
		size_t offset_186_code = 0;
		size_t offset_186_message = 0;
		size_t offset_186_num_orders = 0;

		size_t index_99 = 0;
		size_t offset_99_queue_size = 0; //i4
		size_t offset_99_penalty_remain = 0; //i4
		size_t offset_99_message = 0; //c128

		size_t index_100 = 0;
		size_t offset_100_code = 0; //i4
		size_t offset_100_message = 0; //c255

		if (cg_lsn_getscheme(listener, &scheme) == CG_ERR_OK)
		{
			size_t msgidx = 0;
			for (cg_message_desc_t* msgdesc = scheme->messages; msgdesc; msgdesc = msgdesc->next, msgidx++)
			{
				size_t fieldindex = 0;

				if (strcmp(msgdesc->name, "FORTS_MSG179") == 0)
				{
					index_179 = msgidx;
					for (cg_field_desc_t* fielddesc = msgdesc->fields; fielddesc; fielddesc = fielddesc->next, fieldindex++)
					{
						if (strcmp(fielddesc->name, "code") == 0 && strcmp(fielddesc->type, "i4") == 0)
						{
							offset_179_code = fielddesc->offset;
						}
						else if (strcmp(fielddesc->name, "message") == 0 && strcmp(fielddesc->type, "c255") == 0)
						{
							offset_179_message = fielddesc->offset;
						}
						else if (strcmp(fielddesc->name, "order_id") == 0 && strcmp(fielddesc->type, "i8") == 0)
						{
							offset_179_order_id = fielddesc->offset;
						}
					}
				}
				else if (strcmp(msgdesc->name, "FORTS_MSG186") == 0)
				{
					index_186 = msgidx;
					for (cg_field_desc_t* fielddesc = msgdesc->fields; fielddesc; fielddesc = fielddesc->next, fieldindex++)
					{
						if (strcmp(fielddesc->name, "code") == 0 && strcmp(fielddesc->type, "i4") == 0)
						{
							offset_186_code = fielddesc->offset;
						}
						else if (strcmp(fielddesc->name, "message") == 0 && strcmp(fielddesc->type, "c255") == 0)
						{
							offset_186_message = fielddesc->offset;
						}
						else if (strcmp(fielddesc->name, "num_orders") == 0 && strcmp(fielddesc->type, "i4") == 0)
						{
							offset_186_num_orders = fielddesc->offset;
						}
					}
				}
				else if (strcmp(msgdesc->name, "FORTS_MSG99") == 0)
				{
					index_99 = msgidx;
					for (cg_field_desc_t* fielddesc = msgdesc->fields; fielddesc; fielddesc = fielddesc->next, fieldindex++)
					{
						if (strcmp(fielddesc->name, "queue_size") == 0 && strcmp(fielddesc->type, "i4") == 0)
						{
							offset_99_queue_size = fielddesc->offset;
						}
						else if (strcmp(fielddesc->name, "message") == 0 && strcmp(fielddesc->type, "c128") == 0)
						{
							offset_99_message = fielddesc->offset;
						}
						else if (strcmp(fielddesc->name, "penalty_remain") == 0 && strcmp(fielddesc->type, "i4") == 0)
						{
							offset_99_penalty_remain = fielddesc->offset;
						}
					}
				}
				else if (strcmp(msgdesc->name, "FORTS_MSG100") == 0)
				{
					index_100 = msgidx;
					for (cg_field_desc_t* fielddesc = msgdesc->fields; fielddesc; fielddesc = fielddesc->next, fieldindex++)
					{
						if (strcmp(fielddesc->name, "code") == 0 && strcmp(fielddesc->type, "i4") == 0)
						{
							offset_100_code = fielddesc->offset;
						}
						else if (strcmp(fielddesc->name, "message") == 0 && strcmp(fielddesc->type, "c255") == 0)
						{
							offset_100_message = fielddesc->offset;
						}
					}
				}
			}

			cg_msg_streamdata_t* replmsg = (cg_msg_streamdata_t*)msg;
			char* msg_data = (char*)replmsg->data;

			if (replmsg->msg_index == index_179)
			{
				code = *((int*)(msg_data + offset_179_code));
				strcpy(message, ((char*)(msg_data + offset_179_message)));
				order_id = *((long long*)(msg_data + offset_179_order_id));

				cg_log_info("%s. Code: %d. Message: %s. Order id: %lld.",
					isin, code, message, order_id);

				if (code == 332)
				{
					code_332 = true;
				}
				else if(code == 0 && code_332)
				{
					code_332 = false;
				}
			}
			else if (replmsg->msg_index == index_186)
			{
				code = *((int*)(msg_data + offset_186_code));
				strcpy(message, ((char*)(msg_data + offset_186_message)));
				order_id = *((int*)(msg_data + offset_186_num_orders));

				cg_log_info("%s. Code: %d. Message: %s. Order num: %lld.",
					isin, code, message, order_id);
			}
			else if (replmsg->msg_index == index_99)
			{
				code = *((int*)(msg_data + offset_99_queue_size));
				strcpy(message, ((char*)(msg_data + offset_99_message)));
				order_id = *((int*)(msg_data + offset_99_penalty_remain));

				cg_log_info("%s. Code: %d. Message: %s. Num: %lld.",
					isin, code, message, order_id);
			}
			else if (replmsg->msg_index == index_100)
			{
				code = *((int*)(msg_data + offset_100_code));
				strcpy(message, ((char*)(msg_data + offset_100_message)));
				order_id = 0;

				cg_log_info("%s. Code: %d. Message: %s. Num: %lld.",
					isin, code, message, order_id);
			}
			else
			{
				size_t bufSize;

				if (cg_msg_dump(msg, scheme, 0, &bufSize) == CG_ERR_BUFFERTOOSMALL)
				{
					char* buffer = (char*)malloc(bufSize + 1);
					bufSize++;
					if (cg_msg_dump(msg, scheme, buffer, &bufSize) == CG_ERR_OK)
						cg_log_info("client dump: %s\n", buffer);
					free(buffer);
				}

			}
		}
		else
		{
			scheme = 0;
			size_t bufSize;
			if (cg_msg_dump(msg, scheme, 0, &bufSize) == CG_ERR_BUFFERTOOSMALL)
			{
				char* buffer = (char*)malloc(bufSize + 1);

				bufSize++;
				if (cg_msg_dump(msg, scheme, buffer, &bufSize) == CG_ERR_OK)
					cg_log_info("client dump: %s\n", buffer);
				free(buffer);
			}
		}

		break;
	}
	case CG_MSG_P2MQ_TIMEOUT:
	{
		cg_log_info("Client reply TIMEOUT\n");
		break;
	}
	default:
		cg_log_info("Message 0x%X\n", msg->type);
	}
	return CG_ERR_OK;
};
CG_RESULT OrderSend(cg_publisher_t* publisher_trade, uint32_t counter, int32_t dir, int32_t volume, double price, int32_t type)
{
	int32_t result = 0;
	struct cg_msg_data_t* add_order_msg{ 0 };
	struct AddOrder* add_order{ 0 };
	result = cg_pub_msgnew(publisher_trade, CG_KEY_NAME, "AddOrder", (struct cg_msg_t**)&add_order_msg);

	memcpy(add_order_msg->data, &counter, sizeof(counter));
	add_order_msg->user_id = counter;

	add_order = (struct AddOrder*)add_order_msg->data;
	memset(add_order, 0, sizeof(struct AddOrder));
	strcpy(add_order->broker_code, broker_code);
	strcpy(add_order->client_code, client_code);
	add_order->isin_id = fut_isin_id;
	strcpy(add_order->comment, "");
	strcpy(add_order->broker_to, "");
	add_order->ext_id = 0;
	add_order->is_check_limit = 0;
	strcpy(add_order->date_exp, "");
	add_order->dont_check_money = 0;
	strcpy(add_order->match_ref, "");
	add_order->ncc_request = 0;

	add_order->type = type;
	add_order->dir = dir;
	add_order->amount = volume;
	//sprintf(add_order->price, "%f", price);
	sprintf(add_order->price, price_format, price);

	result = cg_pub_post(publisher_trade, (struct cg_msg_t*)add_order_msg, CG_PUB_NEEDREPLY);
	result = cg_pub_msgfree(publisher_trade, (struct cg_msg_t*)add_order_msg);

	cg_log_info("%s: Send order %s; volume: %d; price: %G(%G); type: %s",
		isin, dir == sell ? "sell" : "buy", volume, price, dir == sell ? price + pdev : price - pdev, 
		type == iok ? "IOK" : type == fok ? "FOK" : "RETURN");

	return(result);
}
void Save(const char* fn, double* price, size_t count)
{
	FILE* fh = 0;
	fh = fopen(fn, "wb");

	if (fh)
	{
		fwrite(price, sizeof(double), count, fh);
		fclose(fh);
	}
}
void Load(const char* fn, double* price, size_t count)
{
	FILE* fh = 0;
	fh = fopen(fn, "rb");

	if (fh)
	{
		fread(price, sizeof(double), count, fh);
		fclose(fh);

		printf("%s: Load done, open price: %G\n", g_cserver_time, *price);
	}
}

int main(int argc, char* argv[])
{	
	if (argc != 15)
	{
		printf("Bot version: %s\n"
			"Use: %s isin, f-alfa, s-alfa, period, size, mtpl, ntr, amount, start, end, close, ref-key, fut-key, trd-key\n",
			version, argv[0]);
		return (0);
		
	}
	
	char refr[CSIZE] = {};
	char fut[CSIZE] = {};
	char trd[CSIZE] = {};

	strcpy(isin, argv[1]);
	double fa = atof(argv[2]);
	double sa = atof(argv[3]);
	int period = atoi(argv[4]);
	int size = atoi(argv[5]);
	int mtpl = atoi(argv[6]);
	int ntr = atoi(argv[7]);
	int amount = atoi(argv[8]);
	int start_trade = atoi(argv[9]);
	int end_trade = atoi(argv[10]);
	int close_bot = atoi(argv[11]);
	strcpy(refr, argv[12]);
	strcpy(fut, argv[13]);
	strcpy(trd, argv[14]);

	int start_clr_1 = 14 * 3600;
	int end_clr_1 = 14 * 3600 + 5 * 60;
	int start_clr_2 = 18 * 3600 + 45 * 60;
	int end_clr_2 = 19 * 3600 + 5 * 60;

	printf("\nBot (%s) starting: ISIN: %s; Parameters:\n"
		"f-Alfa: %G; s-Alfa: %G; Period: %d; Size: %d; MTPL: %d; NTR: %d; Amount: %d;\n"
		"Start: %d:%02d; End: %d:%02d; Close: %d:%02d\n\n",
		version, isin, fa, sa, period, size, mtpl, ntr, amount, 
		start_trade / 3600, (start_trade % 3600) / 60, 
		end_trade / 3600, (end_trade % 3600) / 60, 
		close_bot / 3600, (close_bot % 3600) / 60);

	CFNN n;
	CData d;
	CTrade t;

	cg_listener_t* listener_ref = 0;
	cg_listener_t* listener_hb = 0;
	cg_listener_t* listener_cid = 0;
	cg_listener_t* listener_mid = 0;
	cg_listener_t* listener_part = 0;
	cg_listener_t* listener_pos = 0;
	cg_listener_t* listener_fut = 0;
	cg_listener_t* listener_msg = 0;

	cg_conn_t* conn_ref;
	cg_conn_t* conn_fut;
	cg_conn_t* conn_trd;

	cg_publisher_t* publisher_trade;
	cg_publisher_t* publisher_hb;

	uint32_t state = 0;
	int result = 0;
	long long revision = 0;
	uint32_t counter = 0;
	char c[200];
	clock_t current_moment = 0;
	int current_sec = 0;
	int target_xpos = 0;
	clock_t transaction_moment = 0;
	bool trade_flag = true;
	clock_t cod_last_sec = 0;
	clock_t lsn_last_sec = 0;

	vector <double> x;
	SNetOut nout;
	vector <STradeInfo> t_info;
	bool r_pos = false;

	tm* _tm;
	time_t _time = 0;
	bool _start = false;

	signal(SIGINT, InterruptHandler);
	char key[33] = { };
	sprintf(c, "ini=bot.ini;minloglevel=info;subsystems=mq,replclient;key=%s", key);

	result = cg_env_open(c);

	if (result != CG_ERR_OK)
	{
		return(-1);
	}

	sprintf(c, "p2lrpcq://127.0.0.1:4001;app_name=%s;local_pass=1234;timeout=5000;local_timeout=500;lrpcq_buf=0;name=ref", refr);
	result = cg_conn_new(c, &conn_ref);

	result = cg_lsn_new(conn_ref, "p2repl://FORTS_REFDATA_REPL;tables=fut_sess_contents;name=ref_info", &MessageCallbackFut, &revision, &listener_ref);
	result = cg_lsn_new(conn_ref, "p2repl://FORTS_TRADE_REPL;tables=heartbeat;name=hb_info", &MessageCallbackHB, &revision, &listener_hb);
	result = cg_lsn_new(conn_ref, "p2repl://FORTS_REFDATA_REPL;tables=fut_vcb;name=bcid_info", &MessageCallbackBCID, &revision, &listener_cid);
	result = cg_lsn_new(conn_ref, "p2repl://FORTS_REFDATA_REPL;tables=instr2matching_map;name=mid_info", &MessageCallbackMID, &revision, &listener_mid);
	result = cg_lsn_new(conn_ref, "p2repl://FORTS_PART_REPL;tables=part;name=part_info", &MessageCallbackPart, &revision, &listener_part);
	result = cg_lsn_new(conn_ref, "p2repl://FORTS_POS_REPL;tables=position;name=position_info", &MessageCallbackPos, &revision, &listener_pos); //refer

	sprintf(c, "p2lrpcq://127.0.0.1:4001;app_name=%s;local_pass=1234;timeout=5000;local_timeout=500;lrpcq_buf=0;name=futures_quote", fut);
	result = cg_conn_new(c, &conn_fut);

	sprintf(c, "p2lrpcq://127.0.0.1:4001;app_name=%s;local_pass=1234;timeout=5000;local_timeout=500;lrpcq_buf=0;name=trade", trd);
	result = cg_conn_new(c, &conn_trd);
	result = cg_pub_new(conn_trd, "p2mq://FORTS_SRV;category=FORTS_MSG;name=trade_gate;timeout=5000", &publisher_trade);
	result = cg_pub_new(conn_trd, "p2mq://FORTS_SRV;category=FORTS_MSG;name=cod_hb;timeout=5000", &publisher_hb);
	result = cg_lsn_new(conn_trd, "p2mqreply://;ref=trade_gate;name=trade_msg", &MessageCallbackMSG, 0, &listener_msg);

	// hb, mid
	while (!is_thread_stop && !mid_is_ready)
	{
		result = cg_conn_getstate(conn_ref, &state);
		switch (state)
		{
		case CG_STATE_CLOSED:
			result = cg_conn_open(conn_ref, 0);
			break;
		case CG_STATE_ACTIVE:
			result = cg_conn_process(conn_ref, 0, 0);

			// heartbeat
			result = cg_lsn_getstate(listener_hb, &state);
			switch (state)
			{
			case CG_STATE_CLOSED:
				result = cg_lsn_open(listener_hb, 0);
				break;
			case CG_STATE_ERROR:
				result = cg_lsn_close(listener_hb);
				break;
			}

			// mid
			if (hb_is_ready)
			{
				// futures
				result = cg_lsn_getstate(listener_ref, &state);
				switch (state)
				{
				case CG_STATE_CLOSED:
					result = cg_lsn_open(listener_ref, 0);
					break;
				case CG_STATE_ERROR:
					result = cg_lsn_close(listener_ref);
					break;
				}

				// bcid
				if (!bcid_is_ready && bcc_is_ready)
				{
					result = cg_lsn_getstate(listener_cid, &state);
					switch (state)
					{
					case CG_STATE_CLOSED:
						result = cg_lsn_open(listener_cid, 0);
						break;
					case CG_STATE_ERROR:
						result = cg_lsn_close(listener_cid);
						break;
					}
				}

				// mid
				if (!mid_is_ready && bcid_is_ready)
				{
					result = cg_lsn_getstate(listener_mid, &state);
					switch (state)
					{
					case CG_STATE_CLOSED:
						result = cg_lsn_open(listener_mid, 0);
						break;
					case CG_STATE_ERROR:
						result = cg_lsn_close(listener_mid);
						break;
					}
				}
			}
			break;
		case CG_STATE_ERROR:
			result = cg_conn_close(conn_ref);
			break;
		}
	}
	cg_lsn_close(listener_cid);
	cg_lsn_close(listener_mid);
	cg_lsn_destroy(listener_cid);
	cg_lsn_destroy(listener_mid);
	sprintf(c, "p2repl://FORTS_COMMON_REPL_MATCH%d;tables=common;name=futures_quote_info", matching_id);
	result = cg_lsn_new(conn_fut, c, &MessageCallbackQuote, &revision, &listener_fut);

	d.Init(size, fa, sa, period);
	n.Init(d.dimension, "nns.bin", true);
	t.Init(mtpl, step_size, step_cost);

	// main process
	while (!is_thread_stop)
	{
		current_moment = clock();

		// fut quote process
		result = cg_conn_getstate(conn_fut, &state);
		switch (state)
		{
		case CG_STATE_CLOSED:
			result = cg_conn_open(conn_fut, 0);
			break;
		case CG_STATE_ACTIVE:
			result = cg_conn_process(conn_fut, 0, 0);
			result = cg_lsn_getstate(listener_fut, &state);
			switch (state)
			{
			case CG_STATE_CLOSED:
				result = cg_lsn_open(listener_fut, 0);
				break;
			case CG_STATE_ERROR:
				result = cg_lsn_close(listener_fut);
				break;
			}
			break;
		case CG_STATE_ERROR:
			result = cg_conn_close(conn_fut);
			break;
		}

		// trade process
		result = cg_conn_getstate(conn_trd, &state);
		switch (state)
		{
		case CG_STATE_CLOSED:
			result = cg_conn_open(conn_trd, 0);
			break;
		case CG_STATE_ACTIVE:
			result = cg_conn_process(conn_trd, 0, 0);
			result = cg_pub_getstate(publisher_trade, &state);

			// bot process
			switch (state)
			{
			case CG_STATE_CLOSED:
				result = cg_pub_open(publisher_trade, 0);
				break;
			case CG_STATE_ACTIVE:

				//current_sec = (server_time.hour * 3600 + server_time.minute * 60 + server_time.second);

				_time = sec;
				_tm = localtime(&_time);
				current_sec = (_tm->tm_hour * 3600 + _tm->tm_min * 60 + _tm->tm_sec);

				//trade flag reset
				if (!trade_flag && ((target_xpos < 0 && xpos < 0) || (target_xpos > 0 && xpos > 0) ||
					(target_xpos == 0 && xpos == 0) || double(current_moment - transaction_moment) >= (double)CLOCKS_PER_SEC))
				{
					trade_flag = true;
					cg_log_info("------- TRADE PROCESS TAT: %G sec -------", double(current_moment - transaction_moment) / (double)CLOCKS_PER_SEC);
				}

				// auto trade
				//if (session_state == 1)
				{
					//if(current_sec >= start_trade && current_sec < end_trade)
					if ((current_sec >= start_trade && current_sec < start_clr_1) ||
						(current_sec >= end_clr_1 && current_sec < start_clr_2) || 
						(current_sec >= end_clr_2 && current_sec < end_trade))
					{
						if (!_start)
						{
							_start = true;
							printf("%s: Start trade!\n", g_cserver_time);
						}
						
						if (ask > 0 && bid > 0)
						{
							// tracking
							if (t.Tracking(bid, ask, current_sec, &t_info, &r_pos))
							{
								if (r_pos)
								{
									OrderSend(publisher_trade, counter, xpos > 0 ? sell : buy, int32_t(abs(xpos)),
										xpos > 0 ? bid - pdev : ask + pdev, ret);

									transaction_moment = clock();
									target_xpos = 0;
									trade_flag = false;

									printf("%s: <-- Close %s position, price: %G\n", g_cserver_time,
										xpos > 0 ? "buy" : "sell", xpos > 0 ? bid : ask);
								}
								n.SetRating(&t_info);
							}
							//--- process network
							if (d.Vector(&x, bid, ask, current_sec))
							{
								n.Calc(&x, &nout, sec);
								//--- training network
								if (nout.out < threshold)
								{
									nout.dir = buy;
									nout.id = n.Training(&x, nout.dir, sec);
									t.Open(nout.dir, nout.id, bid, ask, amount, true, current_sec);
									n.SetBuzy(nout.id, true);

									nout.dir = sell;
									nout.id = n.Training(&x, nout.dir, sec);
									t.Open(nout.dir, nout.id, bid, ask, amount, true, current_sec);
									n.SetBuzy(nout.id, true);
								}
								else 
								if (nout.out >= threshold && !nout.buzy)
								{
									//--- trade process
									bool f = !(t.pos_dir == 0 && nout.rating >= ntr);
									if (!f)
									{
										if (amount * nout.dir == buy ? buy_deposit : sell_deposit <= money_free * 0.8)
										{
											OrderSend(publisher_trade, counter, nout.dir, amount, 
												nout.dir == sell ? bid - pdev : ask + pdev, ret);

											transaction_moment = clock();
											cg_log_info("------- CYCLE PROCESS TAT: %G sec -------",
												double(transaction_moment - current_moment) / (double)CLOCKS_PER_SEC);

											target_xpos = nout.dir == sell ? -amount : amount;
											trade_flag = false;

											time_t _time = nout.date;
											tm* _tm = localtime(&_time);
											strftime(c, 200, "%Y.%m.%d %T", _tm);

											printf("%s: --> Open %s position, volume: %d, price: %G; ID: %d; R: %d; D: %s (%ld)\n", 
												g_cserver_time, nout.dir == sell ? "sell" : "buy", amount,
												nout.dir == sell ? bid : ask, nout.id, nout.rating, c, nout.date);
										}
										else
										{
											printf("%s: !!! Not enough money for send %s order, price: %G\n", g_cserver_time,
												nout.dir == sell ? "sell" : "buy", nout.dir == sell ? bid : ask);
										}
									}
									t.Open(nout.dir, nout.id, bid, ask, amount, f, current_sec);
									n.SetBuzy(nout.id, true);
								}
							}
						}
					}
					else if (trade_flag && current_sec >= end_trade)
					{
						if (t.Close(bid, ask, &t_info, &r_pos))
						{
							if (r_pos)
							{
								//end trade by time
								OrderSend(publisher_trade, counter, xpos > 0 ? sell : buy, int32_t(abs(xpos)),
									xpos > 0 ? bid - pdev : ask + pdev, ret);

								transaction_moment = clock();
								target_xpos = 0;
								trade_flag = false;

								printf("%s: <-- Close %s position by end trade, price: %G\n", g_cserver_time,
									xpos > 0 ? "buy" : "sell", xpos > 0 ? bid : ask);
							}
							n.SetRating(&t_info);
						}
					}
				}

				// close bot
				if (current_sec >= close_bot)
				{
					is_thread_stop = true;
				}

				// trade report
				if (current_moment - lsn_last_sec > CLOCKS_PER_SEC)
				{
					lsn_last_sec = current_moment;
					result = cg_lsn_getstate(listener_msg, &state);
					switch (state)
					{
					case CG_STATE_CLOSED:
						result = cg_lsn_open(listener_msg, 0);
						break;
					case CG_STATE_ACTIVE:
						break;
					case CG_STATE_ERROR:
						result = cg_lsn_close(listener_msg);
						break;
					}
				}
				break;
			case CG_STATE_ERROR:
				result = cg_pub_close(publisher_trade);
				break;
			}

			// COD process
			if (current_moment - cod_last_sec > CLOCKS_PER_SEC * 9)
			{
				cod_last_sec = current_moment;

				struct cg_msg_data_t* msg{ 0 };
				result = cg_pub_getstate(publisher_hb, &state);
				switch (state)
				{
				case CG_STATE_CLOSED:
					result = cg_pub_open(publisher_hb, 0);
					break;
				case CG_STATE_OPENING:
					break;
				case CG_STATE_ACTIVE:
					result = cg_pub_msgnew(publisher_hb, CG_KEY_NAME, "CODHeartbeat", (struct cg_msg_t**)&msg);

					memcpy(msg->data, &counter, sizeof(counter));
					msg->user_id = counter;

					struct CODHeartbeat* hb;
					hb = (struct CODHeartbeat*)msg->data;
					memset(hb, 0, sizeof(struct CODHeartbeat));
					hb->seq_number = 0;

					result = cg_pub_post(publisher_hb, (struct cg_msg_t*)msg, 0);
					result = cg_pub_msgfree(publisher_hb, (struct cg_msg_t*)msg);
					break;
				case CG_STATE_ERROR:
					result = cg_pub_close(publisher_hb);
					break;
				}
			}
			break;
		case CG_STATE_ERROR:
			result = cg_conn_close(conn_trd);
			break;
		}

		// refer process
		result = cg_conn_getstate(conn_ref, &state);
		switch (state)
		{
		case CG_STATE_CLOSED:
			result = cg_conn_open(conn_ref, 0);
			break;
		case CG_STATE_ACTIVE:
			result = cg_conn_process(conn_ref, 0, 0);

			// heartbeat
			result = cg_lsn_getstate(listener_hb, &state);
			switch (state)
			{
			case CG_STATE_CLOSED:
				result = cg_lsn_open(listener_hb, 0);
				break;
			case CG_STATE_ERROR:
				result = cg_lsn_close(listener_hb);
				break;
			}

			// position
			result = cg_lsn_getstate(listener_pos, &state);
			switch (state)
			{
			case CG_STATE_CLOSED:
				result = cg_lsn_open(listener_pos, 0);
				break;
			case CG_STATE_ERROR:
				result = cg_lsn_close(listener_pos);
				break;
			}

			// part
			result = cg_lsn_getstate(listener_part, &state);
			switch (state)
			{
			case CG_STATE_CLOSED:
				result = cg_lsn_open(listener_part, 0);
				break;
			case CG_STATE_ERROR:
				result = cg_lsn_close(listener_part);
				break;
			}

			// futures
			result = cg_lsn_getstate(listener_ref, &state);
			switch (state)
			{
			case CG_STATE_CLOSED:
				result = cg_lsn_open(listener_ref, 0);
				break;
			case CG_STATE_ERROR:
				result = cg_lsn_close(listener_ref);
				break;
			}

			break;
		case CG_STATE_ERROR:
			result = cg_conn_close(conn_ref);
			break;
		}
	}

	// out process
	d.DeInit();
	t.DeInit();
	n.Reset();
	n.DeInit();

	result = cg_lsn_close(listener_ref);
	result = cg_lsn_destroy(listener_ref);
	result = cg_lsn_close(listener_hb);
	result = cg_lsn_destroy(listener_hb);
	result = cg_lsn_close(listener_part);
	result = cg_lsn_destroy(listener_part);
	result = cg_lsn_close(listener_pos);
	result = cg_lsn_destroy(listener_pos);
	result = cg_lsn_close(listener_fut);
	result = cg_lsn_destroy(listener_fut);
	result = cg_lsn_close(listener_msg);
	result = cg_lsn_destroy(listener_msg);
	result = cg_pub_close(publisher_trade);
	result = cg_pub_destroy(publisher_trade);
	result = cg_pub_close(publisher_hb);
	result = cg_pub_destroy(publisher_hb);

	result = cg_conn_close(conn_trd);
	result = cg_conn_destroy(conn_trd);
	result = cg_conn_close(conn_fut);
	result = cg_conn_destroy(conn_fut);
	result = cg_conn_close(conn_ref);
	result = cg_conn_destroy(conn_ref);

	cg_env_close();

	printf("%s: Bot shutdown\n", g_cserver_time);

	return(0);
};
