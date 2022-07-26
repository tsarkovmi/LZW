#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Windows.h>
#include <locale.h>
#include <time.h>

#define CODE_BIT		12
#define END_FILE_MARK	4095
#define MAX_CODE		4094
#define TBL_SIZE		5021

FILE *in, *out;

int *Table_code;			//коды
unsigned int *prefix_code;	//доп.коды
unsigned char *Add_char;	//добавленные символы
int count_char = 1, count_out = 0;	//счетчики


int bit_count = 0;
unsigned long bit_buf = 0L;

unsigned char tmp_decode[4000];

//====================================================

//побитная запись кода в файл
void WriteBinFile(unsigned int code)
{
	bit_buf |= (unsigned long)code << (32 - CODE_BIT - bit_count);
	bit_count += CODE_BIT;
	while (bit_count >= 8)
	{
		putc(bit_buf >> 24, out);
		bit_buf <<= 8;
		bit_count -= 8;
		count_out++;
	}
}
//побитное чтение кода из файла
unsigned int ReadBinFile(void)
{
	unsigned int return_value;

	while (bit_count <= 24)
	{
		bit_buf |= (unsigned long)getc(in) << (24 - bit_count);
		bit_count += 8;
	}
	return_value = bit_buf >> (32 - CODE_BIT);
	bit_buf <<= CODE_BIT;
	bit_count -= CODE_BIT;
	return return_value;
}
//поиск кода
int search_string(int hash_prefix, unsigned int hash_character)
{
	int i, j;

	i = (hash_character << 4);	//12 - 8 = 4
	i = i ^ hash_prefix;
	if (i == 0) j = 1;
	else j = TBL_SIZE - i;
	while (1)
	{
		if (Table_code[i] == -1) return i;
		if (prefix_code[i] == hash_prefix && Add_char[i] == hash_character) return i;
		i -= j;
		if (i < 0) i += TBL_SIZE;
	}
}
//кодирование
void coding(void)
{
	unsigned int tmp_max_code = 256;
	unsigned int chi;
	unsigned int string_code;
	unsigned int i;

	for (i = 0; i < TBL_SIZE; i++) Table_code[i] = -1;

	string_code = getc(in);

	while ((chi = getc(in)) != (unsigned)EOF)
	{
		count_char++;
		i = search_string(string_code, chi);
		if (Table_code[i] != -1)
			string_code = Table_code[i];	//такая строка уже есть
		else									//такой строки нет - добавляем
		{
			if (tmp_max_code <= MAX_CODE)
			{
				Table_code[i] = tmp_max_code++;
				prefix_code[i] = string_code;
				Add_char[i] = chi;
			}
			WriteBinFile(string_code);	//выведем сначала последнюю строку
			string_code = chi;				//добавим новую
		}
	}
	WriteBinFile(string_code);
	WriteBinFile(END_FILE_MARK);    //метка конца файла кода
	WriteBinFile(0);	//завершение
}

//разборка строки кода
unsigned char * decode_string(unsigned char *ptmp, unsigned int code)
{
	while (code > 255)
	{
		*ptmp++ = Add_char[code];
		code = prefix_code[code];
	}
	*ptmp = code;
	return ptmp;
}
//декодирование
void decoding()
{
	unsigned int tmp_max_code = 256;
	unsigned int new_code;
	unsigned int old_code;
	int chi;
	unsigned char *pstr;

	old_code = ReadBinFile();
	chi = old_code;
	putc(old_code, out);

	while ((new_code = ReadBinFile()) != (END_FILE_MARK))
	{
		if (new_code >= tmp_max_code)		//еще не было
		{
			*tmp_decode = chi;
			pstr = decode_string(tmp_decode + 1, old_code);
		}
		else							//декодируем
			pstr = decode_string(tmp_decode, new_code);

		chi = *pstr;
		while (pstr >= tmp_decode)		//записываем в файл
		{
			putc(*pstr, out);
			pstr--;
		}

		if (tmp_max_code <= MAX_CODE)
		{
			prefix_code[tmp_max_code] = old_code;
			Add_char[tmp_max_code] = chi;
			tmp_max_code++;
		}
		old_code = new_code;
	}
}
//========================================================================

int main(int argc, char *argv[])
{
	double sec;
	clock_t start, finish;

	setlocale(LC_ALL, "Rus");
	//SetConsoleCP (1251); SetConsoleOutputCP(1251);

	if (argc != 4)
	{
		printf("Парамеры запуска через пробел.\n");
		printf("Введите режим: 0-Кодирование, 1-Декодирование\nИмя входного файла\nИмя выходного файла");
		return 1;
	}

	if (*argv[1] == '0')	//кодирование
	{
		if ((in = fopen(argv[2], "rb")) == NULL) { printf("Ошибка открытия входного файла!\n"); return 1; }
		if ((out = fopen(argv[3], "wb")) == NULL) { printf("Ошибка открытия выходного файла!\n"); return 1; }


		Table_code = (int *)malloc(TBL_SIZE * sizeof(int));
		if (Table_code == NULL) { printf("Out of memory!\n"); return 1; }

		prefix_code = (unsigned int *)malloc(TBL_SIZE * sizeof(unsigned int));
		if (prefix_code == NULL) { printf("Out of memory!\n"); return 1; }

		Add_char = (unsigned char *)malloc(TBL_SIZE * sizeof(unsigned char));
		if (Add_char == NULL) { printf("Out of memory!\n"); return 1; }

		//in=fopen("test.txt","rb");
		//out=fopen("test.lzw","wb");

		start = clock();
		coding();
		finish = clock();
		sec = (double)(finish - start) / (double)CLOCKS_PER_SEC;
		printf("Время кодирования:%7.3f сек На входе:%7d байт На выходе:%7d байт\n", sec, count_char, count_out);
		fclose(in);
		fclose(out);
		free(Table_code);
		free(prefix_code);
		free(Add_char);
	}

	else
	{
		if ((in = fopen(argv[2], "rb")) == NULL) { printf("Ошибка открытия входного файла!\n"); return 1; }
		if ((out = fopen(argv[3], "wb")) == NULL) { printf("Ошибка открытия выходного файла!\n"); return 1; }

		prefix_code = (unsigned int *)malloc(TBL_SIZE * sizeof(unsigned int));
		if (prefix_code == NULL) { printf("Out of memory!\n"); return 1; }

		Add_char = (unsigned char *)malloc(TBL_SIZE * sizeof(unsigned char));
		if (Add_char == NULL) { printf("Out of memory!\n"); return 1; }

		//	bit_count=0;
		//	bit_buf=0L;

		//	in=fopen("test.lzw","rb");
		//	out=fopen("test.out","wb");

		start = clock();
		decoding();
		finish = clock();
		sec = (double)(finish - start) / (double)CLOCKS_PER_SEC;
		printf("Время декодирования: %7.3f сек\n", sec);

		fclose(in);
		fclose(out);
		free(prefix_code);
		free(Add_char);
	}
}
