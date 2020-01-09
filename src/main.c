/*
 ============================================================================
 Project Name: project_name
 Name        : file_name.c
 Author      : d-logic (http://www.d-logic.net/nfc-rfid-reader-sdk/)
 Version     :
 Copyright   : 2017.
 Description : Project in C (Language standard: c99)
 Dependencies: uFR firmware - min. version x.y.z {define in ini.h}
               uFRCoder library - min. version x.y.z {define in ini.h}
 ============================================================================
 */

/* includes:
 * stdio.h & stdlib.h are included by default (for printf and LARGE_INTEGER.QuadPart (long long) use %lld or %016llx).
 * inttypes.h, stdbool.h & string.h included for various type support and utilities.
 * conio.h is included for windows(dos) console input functions.
 * windows.h is needed for various timer functions (GetTickCount(), QueryPerformanceFrequency(), QueryPerformanceCounter())
 */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#if __WIN32 || __WIN64
#	include <conio.h>
#	include <windows.h>
#elif linux || __linux__ || __APPLE__
#	define __USE_MISC
#	include <unistd.h>
#	include <termios.h>
#	undef __USE_MISC
#	include "conio_gnu.h"
#else
#	error "Unknown build platform."
#endif
#include <uFCoder.h>
#include "ini.h"
#include "uFR.h"
#include "utils.h"
//------------------------------------------------------------------------------
void usage(void);
void menu(char key);
UFR_STATUS NewCardInField(uint8_t sak, uint8_t *uid, uint8_t uid_size);
void page_read(void);
void page_write(void);
void page_in_sector_read(void);
void page_in_sector_write(void);
void linear_read(void);
void linear_write(void);
void pwd_pack_write(void);
void keys_lock(void);
void keys_unlock(void);

//------------------------------------------------------------------------------
int main(void)
{
	char key;
	bool card_in_field = false;
	uint8_t old_sak = 0, old_uid_size = 0, old_uid[10];
	uint8_t sak, uid_size, uid[10];
	UFR_STATUS status;


	printf(" --------------------------------------------------\n");
	printf("     Please wait while opening uFR NFC reader.\n");
	printf(" --------------------------------------------------\n");

	int mode = 0;
	printf("Select reader opening mode:\n");
	printf(" (1) - Simple Reader Open\n");
	printf(" (2) - Advanced Reader Open\n");
	scanf("%d", &mode);
	fflush(stdin);

	if (mode == 1){
		status = ReaderOpen();
	}
	else if (mode == 2)
	{
	   uint32_t reader_type = 1;
	   char port_name[1024] = "";
	   uint32_t port_interface = 2;
	   char open_args[1024] = "";
	   const char str_interface[2] = "";

	   printf("Enter reader type:\n");
	   scanf("%d", &reader_type);
	   fflush(stdin);

	   printf("Enter port name:\n");
	   scanf("%s", port_name);
	   fflush(stdin);

	   printf("Enter port interface:\n");
	   scanf("%s", str_interface);
	   if (str_interface[0] == 'U'){
		   port_interface = 85;
	   } else if (str_interface[0] == 'T'){
		   port_interface = 84;
	   } else{
		   port_interface = atoi(str_interface);
	   }

	   fflush(stdin);

	   printf("Enter additional argument:\n");
	   scanf("%s", open_args);
	   fflush(stdin);

	   status = ReaderOpenEx(reader_type, port_name, port_interface, open_args);

	}
	else
	{
		printf("Invalid input. Press any key to quit the application...");
		getchar();
		return EXIT_FAILURE;
	}

	if (status != UFR_OK)
	{
		printf("Error while opening device, status is: 0x%08X\n", status);
		getchar();
		return EXIT_FAILURE;
	}

#if __WIN32 || __WIN64
	Sleep(500);
#else // if linux || __linux__ || __APPLE__
	usleep(500000);
#endif

	if (!CheckDependencies())
	{
		ReaderClose();
		getchar();
		return EXIT_FAILURE;
	}

	printf(" --------------------------------------------------\n");
	printf("        uFR NFC reader successfully opened.\n");
	printf(" --------------------------------------------------\n");

#if linux || __linux__ || __APPLE__
	_initTermios(0);
#endif

	usage();

	do
	{
		while (!_kbhit())
		{
			status = GetCardIdEx(&sak, uid, &uid_size);
			switch (status)
			{
				case UFR_OK:
					if (card_in_field)
					{
						if (old_sak != sak || old_uid_size != uid_size || memcmp(old_uid, uid, uid_size))
						{
							old_sak = sak;
							old_uid_size = uid_size;
							memcpy(old_uid, uid, uid_size);
							NewCardInField(sak, uid, uid_size);
						}
					}
					else
					{
						old_sak = sak;
						old_uid_size = uid_size;
						memcpy(old_uid, uid, uid_size);
						NewCardInField(sak, uid, uid_size);
						card_in_field = true;
					}
					break;
				case UFR_NO_CARD:
					card_in_field = false;
					status = UFR_OK;
					break;
				default:
					ReaderClose();
					printf(" Fatal error while trying to read card, status is: 0x%08X\n", status);
					getchar();
#if linux || __linux__ || __APPLE__
					_resetTermios();
					tcflush(0, TCIFLUSH); // Clear stdin to prevent characters appearing on prompt
#endif
					return EXIT_FAILURE;
			}
#if __WIN32 || __WIN64
			Sleep(300);
#else // if linux || __linux__ || __APPLE__
			usleep(300000);
#endif
		}

		key = _getch();
		menu(key);
	}
	while (key != '\x1b');

	ReaderClose();
#if linux || __linux__ || __APPLE__
	_resetTermios();
	tcflush(0, TCIFLUSH); // Clear stdin to prevent characters appearing on prompt
#endif
	return EXIT_SUCCESS;
}
//------------------------------------------------------------------------------
void menu(char key)
{

	switch (key)
	{
		case '1':
			page_read();
			break;

		case '2':
			page_write();
			break;

		case '3':
			page_in_sector_read();
			break;

		case '4':
			page_in_sector_write();
			break;

		case '5':
			linear_read();
			break;

		case '6':
			linear_write();
			break;

		case '7':
			pwd_pack_write();
			break;

		case '8':
			keys_lock();
			break;

		case '9':
			keys_unlock();
			break;

		case '\x1b':
			break;

		default:
			usage();
			break;
	}
}
//------------------------------------------------------------------------------
void usage(void)
{
		printf(" +------------------------------------------------+\n"
			   " |              NT3H tags example                 |\n"
			   " |              version "APP_VERSION"             |\n"
			   " +------------------------------------------------+\n"
			   "                              For exit, hit escape.\n");
		printf(" --------------------------------------------------\n");
		printf("  (1) - Read Page\n"
			   "  (2) - Write Page\n"
			   "  (3) - Read Page In Sector\n"
			   "  (4) - Write Page In Sector\n"
			   "  (5) - Linear Read\n"
			   "  (6) - Linear Write\n"
			   "  (7) - Writing PWD_PACK into reader\n"
			   "  (8) - Keys lock\n"
			   "  (9) - Keys unlock\n");
}
//------------------------------------------------------------------------------
UFR_STATUS NewCardInField(uint8_t sak, uint8_t *uid, uint8_t uid_size)
{
	UFR_STATUS status;
	uint8_t dl_card_type;

	status = GetDlogicCardType(&dl_card_type);
	if (status != UFR_OK)
		return status;

	printf(" \a-------------------------------------------------------------------\n");
	printf(" Card type: %s, sak = 0x%02X, uid[%d] = ", GetDlTypeName(dl_card_type), sak, uid_size);
	print_hex_ln(uid, uid_size, ":");
	printf(" -------------------------------------------------------------------\n");

	return UFR_OK;
}
//------------------------------------------------------------------------------

bool EnterPageData(uint8_t *page_data)
{
	char str[100];
	size_t str_len;
	char key;

	printf(" (1) - ASCI\n"
		   " (2) - HEX\n");

	while (!_kbhit())
		;
	key = _getch();

	if(key == '1')
	{
		printf("Enter 4 ASCI characters\n");
		scanf("%[^\n]%*c", str);
		str_len = strlen(str);
		if(str_len != 4)
		{
			printf("\nYou need to enter 4 characters\n");
			scanf("%[^\n]%*c", str);
			str_len = strlen(str);
			if(str_len != 4)
				return false;
		}
		memcpy(page_data, str, 4);
		return true;
	}
	else if(key == '2')
	{
		printf("Enter 4 hexadecimal bytes\n");
		scanf("%[^\n]%*c", str);
		str_len = hex2bin(page_data, str);
		if(str_len != 4)
		{
			printf("\nYou need to enter 4 hexadecimal numbers with or without spaces or with : as delimiter\n");
			scanf("%[^\n]%*c", str);
			str_len = hex2bin(page_data, str);
			if(str_len != 4)
				return false;
		}
		return true;
	}
	else
		return false;
}
//------------------------------------------------------------------------------
bool EnterLinearData(uint8_t *linear_data, uint16_t *linear_len)
{
	char str[3440];
	size_t str_len;
	char key;

	*linear_len = 0;

	printf(" (1) - ASCI\n"
		   " (2) - HEX\n");

	while (!_kbhit())
		;
	key = _getch();

	if(key == '1')
	{
		printf("Enter ASCI text\n");
		scanf("%[^\n]%*c", str);
		str_len = strlen(str);
		*linear_len = str_len;
		memcpy(linear_data, str, *linear_len);
		return true;
	}
	else if(key == '2')
	{
		printf("Enter hexadecimal bytes\n");
		scanf("%[^\n]%*c", str);
		str_len = hex2bin(linear_data, str);
		*linear_len = str_len;
		return true;
	}
	else
		return false;
}
//----------------------------------------------------------------------------------
bool EnterPwd(uint8_t *pwd)
{
	char str[100];
	size_t str_len;

	scanf("%[^\n]%*c", str);
	str_len = hex2bin(pwd, str);
	if(str_len != 4)
	{
		printf("\nYou need to enter 4 hexadecimal numbers with or without spaces or with : as delimiter\n");
		scanf("%[^\n]%*c", str);
		str_len = hex2bin(pwd, str);
		if(str_len != 4)
			return false;
	}

	return true;
}
//----------------------------------------------------------------------------------
bool EnterPack(uint8_t *pack)
{
	char str[100];
	size_t str_len;

	scanf("%[^\n]%*c", str);
	str_len = hex2bin(pack, str);
	if(str_len != 2)
	{
		printf("\nYou need to enter 2 hexadecimal numbers with or without spaces or with : as delimiter\n");
		scanf("%[^\n]%*c", str);
		str_len = hex2bin(pack, str);
		if(str_len != 2)
			return false;
	}

	return true;
}
//--------------------------------------------------------------------------------------------------
bool EnterPassword(uint8_t *password)
{
	char str[100];
	size_t str_len;
	char key;

	printf(" (1) - ASCI\n"
		   " (2) - HEX\n");

	while (!_kbhit())
		;
	key = _getch();

	if(key == '1')
	{
		printf("Enter 8 ASCI characters\n");
		scanf("%[^\n]%*c", str);
		str_len = strlen(str);
		if(str_len != 8)
		{
			printf("\nYou need to enter 8 characters\n");
			scanf("%[^\n]%*c", str);
			str_len = strlen(str);
			if(str_len != 8)
				return false;
		}
		memcpy(password, str, 8);
		return true;
	}
	else if(key == '2')
	{
		printf("Enter 8 hexadecimal bytes\n");
		scanf("%[^\n]%*c", str);
		str_len = hex2bin(password, str);
		if(str_len != 8)
		{
			printf("\nYou need to enter 8 hexadecimal numbers with or without spaces or with : as delimiter\n");
			scanf("%[^\n]%*c", str);
			str_len = hex2bin(password, str);
			if(str_len != 8)
				return false;
		}
		return true;
	}
	else
		return false;
}
//----------------------------------------------------------------------------------
void page_read(void)
{
	UFR_STATUS status;
	int page_nr_int;
	uint8_t page_nr;
	uint8_t data[16];
	char key;
	uint8_t pwd_pack[6];
	uint8_t pwd_pack_idx;
	int pwd_pack_idx_int;

	printf(" -------------------------------------------------------------------\n");
	printf("                        Page data read                              \n");
	printf(" -------------------------------------------------------------------\n");

	printf("\nEnter page number (0 - 255)\n");
	scanf("%d%*c", &page_nr_int);
	page_nr = page_nr_int;

	printf("\nEnter authentication key mode\n"
			" (1) - No authentication\n"
			" (2) - Provided key\n"
			" (3) - Reader key authentication\n"
			);

	while (!_kbhit())
			;
	key = _getch();

	if(key == '1')
	{
		status = BlockRead(data, page_nr, T2T_WITHOUT_PWD_AUTH, 0);
	}
	else if(key == '2')
	{
		printf("\nEnter PWD (4 hexadecimal bytes)\n");
		if(!EnterPwd(pwd_pack))
		{
			printf("\nPWD error\n");
			return;
		}

		printf("\nEnter PACK (2 hexadecimal bytes)\n");
		if(!EnterPack(&pwd_pack[4]))
		{
			printf("\nPACK error\n");
			return;
		}

		status = BlockRead_PK(data, page_nr, T2T_WITH_PWD_AUTH, pwd_pack);
	}
	else if(key == '3')
	{
		printf("\nEnter PWD_PACK reader index (0 - 31)\n");
		scanf("%d%*c", &pwd_pack_idx_int);
		pwd_pack_idx = pwd_pack_idx_int;

		status = BlockRead(data, page_nr, T2T_WITH_PWD_AUTH, pwd_pack_idx);
	}
	else
	{
		printf("\nWrong choice\n");
		return;
	}

	if(status)
	{
		printf("\nPage read failed\n");
		printf("Error code = %02X\n", status);
	}
	else
	{
		printf("\nPage read successful\n");
		printf("Data = ");
		print_hex_ln(data, 4, " ");
		printf("\n");
	}
}
//-----------------------------------------------------------------------------
void page_write(void)
{
	UFR_STATUS status;
	int page_nr_int;
	uint8_t page_nr;
	uint8_t data[16];
	char key;
	uint8_t pwd_pack[6];
	uint8_t pwd_pack_idx;
	int pwd_pack_idx_int;

	printf(" -------------------------------------------------------------------\n");
	printf("                        Page data write                             \n");
	printf(" -------------------------------------------------------------------\n");

	printf("\nEnter page number (2 - 255)\n");
	scanf("%d%*c", &page_nr_int);
	page_nr = page_nr_int;

	printf("\nEnter page data 4 bytes or characters\n");
	if(!EnterPageData(data))
	{
		printf("\nError while data entry\n");
		return;
	}

	printf("\nEnter authentication key mode\n"
			" (1) - No authentication\n"
			" (2) - Provided key\n"
			" (3) - Reader key authentication\n"
			);

	while (!_kbhit())
			;
	key = _getch();

	if(key == '1')
	{
		status = BlockWrite(data, page_nr, T2T_WITHOUT_PWD_AUTH, 0);
	}
	else if(key == '2')
	{
		printf("\nEnter PWD (4 hexadecimal bytes)\n");
		if(!EnterPwd(pwd_pack))
		{
			printf("\nPWD error\n");
			return;
		}

		printf("\nEnter PACK (2 hexadecimal bytes)\n");
		if(!EnterPack(&pwd_pack[4]))
		{
			printf("\nPACK error\n");
			return;
		}

		status = BlockWrite_PK(data, page_nr, T2T_WITH_PWD_AUTH, pwd_pack);
	}
	else if(key == '3')
	{
		printf("\nEnter PWD_PACK reader index (0 - 31)\n");
		scanf("%d%*c", &pwd_pack_idx_int);
		pwd_pack_idx = pwd_pack_idx_int;

		status = BlockWrite(data, page_nr, T2T_WITH_PWD_AUTH, pwd_pack_idx);
	}
	else
	{
		printf("\nWrong choice\n");
		return;
	}

	if(status)
	{
		printf("\nPage write failed\n");
		printf("Error code = %02X\n", status);
	}
	else
		printf("\nPage write successful\n");
}
//----------------------------------------------------------------------------------
void page_in_sector_read(void)
{
	UFR_STATUS status;
	int page_nr_int;
	int sector_nr_int;
	uint8_t sector_nr;
	uint8_t page_nr;
	uint8_t data[16];
	char key;
	uint8_t pwd_pack[6];
	uint8_t pwd_pack_idx;
	int pwd_pack_idx_int;


	printf(" -------------------------------------------------------------------\n");
	printf("                    Page in sector data read                        \n");
	printf(" -------------------------------------------------------------------\n");

	printf("\nEnter sector number (0 - 3)\n");
	scanf("%d%*c", &sector_nr_int);
	sector_nr = sector_nr_int;

	printf("\nEnter page number (0 - 255)\n");
	scanf("%d%*c", &page_nr_int);
	page_nr = page_nr_int;

	printf("\nEnter authentication key mode\n"
			" (1) - No authentication\n"
			" (2) - Provided key\n"
			" (3) - Reader key authentication\n"
			);

	while (!_kbhit())
			;
	key = _getch();

	if(key == '1')
	{
		status = BlockInSectorRead(data, sector_nr, page_nr, T2T_WITHOUT_PWD_AUTH, 0);
	}
	else if(key == '2')
	{
		printf("\nEnter PWD (4 hexadecimal bytes)\n");
		if(!EnterPwd(pwd_pack))
		{
			printf("\nPWD error\n");
			return;
		}

		printf("\nEnter PACK (2 hexadecimal bytes)\n");
		if(!EnterPack(&pwd_pack[4]))
		{
			printf("\nPACK error\n");
			return;
		}

		status = BlockInSectorRead_PK(data, sector_nr, page_nr, T2T_WITH_PWD_AUTH, pwd_pack);
	}
	else if(key == '3')
	{
		printf("\nEnter PWD_PACK reader index (0 - 31)\n");
		scanf("%d%*c", &pwd_pack_idx_int);
		pwd_pack_idx = pwd_pack_idx_int;

		status = BlockInSectorRead(data, sector_nr, page_nr, T2T_WITH_PWD_AUTH, pwd_pack_idx);
	}
	else
	{
		printf("\nWrong choice\n");
		return;
	}

	if(status)
	{
		printf("\nPage read failed\n");
		printf("Error code = %02X\n", status);
	}
	else
	{
		printf("\nPage read successful\n");
		printf("Data = ");
		print_hex_ln(data, 4, " ");
		printf("\n");
	}
}
//-----------------------------------------------------------------------------
void page_in_sector_write(void)
{
	UFR_STATUS status;
	int page_nr_int;
	int sector_nr_int;
	uint8_t sector_nr;
	uint8_t page_nr;
	uint8_t data[16];
	char key;
	uint8_t pwd_pack[6];
	uint8_t pwd_pack_idx;
	int pwd_pack_idx_int;

	printf(" -------------------------------------------------------------------\n");
	printf("                   Page in sector data write                        \n");
	printf(" -------------------------------------------------------------------\n");

	printf("\nEnter sector number (0 - 3)\n");
	scanf("%d%*c", &sector_nr_int);
	sector_nr = sector_nr_int;

	printf("\nEnter page number (2 - 255)\n");
	scanf("%d%*c", &page_nr_int);
	page_nr = page_nr_int;

	printf("\nEnter page data 4 bytes or characters\n");
	if(!EnterPageData(data))
	{
		printf("\nError while data entry\n");
		return;
	}

	printf("\nEnter authentication key mode\n"
			" (1) - No authentication\n"
			" (2) - Provided key\n"
			" (3) - Reader key authentication\n"
			);

	while (!_kbhit())
			;
	key = _getch();

	if(key == '1')
	{
		status = BlockInSectorWrite(data, sector_nr, page_nr, T2T_WITHOUT_PWD_AUTH, 0);
	}
	else if(key == '2')
	{
		printf("\nEnter PWD (4 hexadecimal bytes)\n");
		if(!EnterPwd(pwd_pack))
		{
			printf("\nPWD error\n");
			return;
		}

		printf("\nEnter PACK (2 hexadecimal bytes)\n");
		if(!EnterPack(&pwd_pack[4]))
		{
			printf("\nPACK error\n");
			return;
		}

		status = BlockInSectorWrite_PK(data, sector_nr, page_nr, T2T_WITH_PWD_AUTH, pwd_pack);
	}
	else if(key == '3')
	{
		printf("\nEnter PWD_PACK reader index (0 - 31)\n");
		scanf("%d%*c", &pwd_pack_idx_int);
		pwd_pack_idx = pwd_pack_idx_int;

		status = BlockInSectorWrite(data, sector_nr, page_nr, T2T_WITH_PWD_AUTH, pwd_pack_idx);
	}
	else
	{
		printf("\nWrong choice\n");
		return;
	}

	if(status)
	{
		printf("\nPage write failed\n");
		printf("Error code = %02X\n", status);
	}
	else
		printf("\nPage write successful\n");
}
//---------------------------------------------------------------------------
void linear_read(void)
{
	UFR_STATUS status;
	uint16_t lin_addr, lin_len, ret_bytes;
	int lin_addr_int, lin_len_int;
	uint8_t data[200];

	printf(" -------------------------------------------------------------------\n");
	printf("                        Linear read                                 \n");
	printf(" -------------------------------------------------------------------\n");

	printf("\nEnter linear address (0 - 143)\n");
	scanf("%d%*c", &lin_addr_int);
	lin_addr = lin_addr_int;

	printf("\nEnter number of bytes for read\n");
	scanf("%d%*c", &lin_len_int);
	lin_len = lin_len_int;

	status = LinearRead(data, lin_addr, lin_len, &ret_bytes, T2T_WITHOUT_PWD_AUTH, 0);

	if(status)
	{
		printf("\nLinear read failed\n");
		printf("Error code = %02X\n", status);
	}
	else
	{
		printf("\nLinear read successful\n");
		printf("Data = ");
		print_hex_ln(data, ret_bytes, " ");
		printf("ASCI = %s\n", data);
	}
}
//----------------------------------------------------------------------------------
void linear_write(void)
{
	UFR_STATUS status;
	uint16_t lin_addr, lin_len, ret_bytes;
	int lin_addr_int, lin_len_int;
	uint8_t data[200];

	printf(" -------------------------------------------------------------------\n");
	printf("                        Linear write                                 \n");
	printf(" -------------------------------------------------------------------\n");

	printf("\nEnter linear address (0 - 143)\n");
	scanf("%d%*c", &lin_addr_int);
	lin_addr = lin_addr_int;

	printf("\nEnter linear data\n");
	if(!EnterLinearData(data, &lin_len))
	{
		printf("\nError while data entry\n");
		return;
	}

	status = LinearWrite(data, lin_addr, lin_len, &ret_bytes, T2T_WITHOUT_PWD_AUTH, 0);

	if(status)
	{
		printf("\nLinear write failed\n");
		printf("Error code = %02X\n", status);
	}
	else
		printf("\nLinear write successful\n");
}
//----------------------------------------------------------------------------------
void pwd_pack_write(void)
{
	UFR_STATUS status;
	int key_index_int;
	uint8_t key_index;
	uint8_t pwd_pack[6];

	printf(" -------------------------------------------------------------------\n");
	printf("                Write PWD and PACK into reader                      \n");
	printf(" -------------------------------------------------------------------\n");

	printf("Enter PWD_PACK in reader number (0 - 31)\n");
	scanf("%d%*c", &key_index_int);
	key_index = key_index_int;

	printf("\nEnter PWD (4 hexadecimal bytes)\n");
	if(!EnterPwd(pwd_pack))
	{
		printf("\nPWD error\n");
		return;
	}

	printf("\nEnter PACK (2 hexadecimal bytes)\n");
	if(!EnterPack(&pwd_pack[4]))
	{
		printf("\nPACK error\n");
		return;
	}

	status = ReaderKeyWrite(pwd_pack, key_index);

	if(status)
	{
		printf("\nReader key writing error\n");
		printf("Error code = %02X\n", status);
	}
	else
		printf("\nKey written into reader successfully\n");
}
//-------------------------------------------------------------------------------------------

void keys_lock(void)
{
	UFR_STATUS status;
	uint8_t password[8];

	printf(" -------------------------------------------------------------------\n");
	printf("                        Reader keys lock                            \n");
	printf(" -------------------------------------------------------------------\n");

	printf("Enter password\n");
	if(!EnterPassword(password))
	{
		printf("Password enter error\n");
		return;
	}

	status = ReaderKeysLock(password);

	if(status)
	{
		printf("\nReader keys lock error\n");
		printf("Error code = %02X\n", status);
	}
	else
		printf("\nReader keys locked successfully\n");
}
//--------------------------------------------------------------------------------------------

void keys_unlock(void)
{
	UFR_STATUS status;
	uint8_t password[8];

	printf(" -------------------------------------------------------------------\n");
	printf("                       Reader keys unlock                           \n");
	printf(" -------------------------------------------------------------------\n");

	printf("Enter password\n");
	if(!EnterPassword(password))
	{
		printf("Password enter error\n");
		return;
	}

	status = ReaderKeysUnlock(password);

	if(status)
	{
		printf("\nReader keys unlock error\n");
		printf("Error code = %02X\n", status);
	}
	else
		printf("\nReader keys unlocked successfully\n");
}
//--------------------------------------------------------------------------------------------
