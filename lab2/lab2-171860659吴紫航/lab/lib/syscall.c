#include "lib.h"
#include "types.h"
/*
 * io lib here
 * 库函数写在这
 */
//static inline int32_t syscall(int num, uint32_t a1,uint32_t a2,
int32_t syscall(int num, uint32_t a1,uint32_t a2,
		uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t ret = 0;
	//Generic system call: pass system call number in AX
	//up to five parameters in DX,CX,BX,DI,SI
	//Interrupt kernel with T_SYSCALL
	//
	//The "volatile" tells the assembler not to optimize
	//this instruction away just because we don't use the
	//return value
	//
	//The last clause tells the assembler that this can potentially
	//change the condition and arbitrary memory locations.

	/*
	XXX Note: ebp shouldn't be flushed
	    May not be necessary to store the value of eax, ebx, ecx, edx, esi, edi
	*/
	uint32_t eax, ecx, edx, ebx, esi, edi;
	uint16_t selector;
	
	asm volatile("movl %%eax, %0":"=m"(eax));
	asm volatile("movl %%ecx, %0":"=m"(ecx));
	asm volatile("movl %%edx, %0":"=m"(edx));
	asm volatile("movl %%ebx, %0":"=m"(ebx));
	asm volatile("movl %%esi, %0":"=m"(esi));
	asm volatile("movl %%edi, %0":"=m"(edi));
	asm volatile("movl %0, %%eax"::"m"(num));
	asm volatile("movl %0, %%ecx"::"m"(a1));
	asm volatile("movl %0, %%edx"::"m"(a2));
	asm volatile("movl %0, %%ebx"::"m"(a3));
	asm volatile("movl %0, %%esi"::"m"(a4));
	asm volatile("movl %0, %%edi"::"m"(a5));
	asm volatile("int $0x80");
	asm volatile("movl %%eax, %0":"=m"(ret));
	asm volatile("movl %0, %%eax"::"m"(eax));
	asm volatile("movl %0, %%ecx"::"m"(ecx));
	asm volatile("movl %0, %%edx"::"m"(edx));
	asm volatile("movl %0, %%ebx"::"m"(ebx));
	asm volatile("movl %0, %%esi"::"m"(esi));
	asm volatile("movl %0, %%edi"::"m"(edi));
	
	asm volatile("movw %%ss, %0":"=m"(selector)); //XXX %ds is reset after iret
	//selector = 16;
	asm volatile("movw %%ax, %%ds"::"a"(selector));
	
	return ret;
}

// API to support format in printf, if you can't understand, use your own implementation!
int dec2Str(int decimal, char *buffer, int size, int count);
int hex2Str(uint32_t hexadecimal, char *buffer, int size, int count);
int str2Str(char *string, char *buffer, int size, int count);

int printf(const char *format,...){
	int i=0; // format index
	char buffer[MAX_BUFFER_SIZE];
	int count=0; // buffer index
	 int index=0; // parameter index
	 void *paraList=(void*)&format; // address of format in stack
	 //int state=0; // 0: legal character; 1: '%'; 2: illegal format
	 int decimal=0;
	 uint32_t hexadecimal=0;
	 char *string=0;
	 char character=0;
	while(format[i]!=0){
        // TODO: support more format %s %d %x and so on
        	if(format[i]=='%'){
			i++;
		  switch(format[i]){
		  	
			case 'd':{
					 paraList+=4;
					 decimal=*(int*)paraList;
					 count=dec2Str(decimal,buffer,MAX_BUFFER_SIZE,count);
					 index++;
					 break;
				}					 
			case '%':{
					 buffer[count]='%';
					 count++;
					 break;
				}
			
			case 'x':{
					 paraList+=4;
					 hexadecimal=*(uint32_t*)paraList;
					 count=hex2Str(hexadecimal,buffer,MAX_BUFFER_SIZE,count);
					 index++;
					 break;
				}
			
			case 'c':{
					 paraList+=4;
					 character=(char)*(int*)paraList;
					 buffer[count]=character;
					 count++;
					 index++;
					 break;
				 }
			case 's':{
					paraList+=4;
					string=*(char**)paraList;
					count=str2Str(string,buffer,MAX_BUFFER_SIZE,count);
					index++;
					break;
				 }			
			default:return -1;break; 
		  }
		}
		else{
			buffer[count]=format[i];
			count++;
		}
		if(count==MAX_BUFFER_SIZE) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)MAX_BUFFER_SIZE, 0, 0);
			count=0;
		}
		i++;
	}
	if(count!=0)
		syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)count, 0, 0);
    return index;
}

int dec2Str(int decimal, char *buffer, int size, int count) {
	int i=0;
	int temp;
	int number[16];

	if(decimal<0){
		buffer[count]='-';
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, 0, 0);
			count=0;
		}
		temp=decimal/10;
		number[i]=temp*10-decimal;
		decimal=temp;
		i++;
		while(decimal!=0){
			temp=decimal/10;
			number[i]=temp*10-decimal;
			decimal=temp;
			i++;
		}
	}
	else{
		temp=decimal/10;
		number[i]=decimal-temp*10;
		decimal=temp;
		i++;
		while(decimal!=0){
			temp=decimal/10;
			number[i]=decimal-temp*10;
			decimal=temp;
			i++;
		}
	}

	while(i!=0){
		buffer[count]=number[i-1]+'0';
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, 0, 0);
			count=0;
		}
		i--;
	}
	return count;
}

int hex2Str(uint32_t hexadecimal, char *buffer, int size, int count) {
	int i=0;
	uint32_t temp=0;
	int number[16];

	temp=hexadecimal>>4;
	number[i]=hexadecimal-(temp<<4);
	hexadecimal=temp;
	i++;
	while(hexadecimal!=0){
		temp=hexadecimal>>4;
		number[i]=hexadecimal-(temp<<4);
		hexadecimal=temp;
		i++;
	}

	while(i!=0){
		if(number[i-1]<10)
			buffer[count]=number[i-1]+'0';
		else
			buffer[count]=number[i-1]-10+'a';
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, 0, 0);
			count=0;
		}
		i--;
	}
	return count;
}

int str2Str(char *string, char *buffer, int size, int count) {
	int i=0;
	while(string[i]!=0){
		buffer[count]=string[i];
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, 0, 0);
			count=0;
		}
		i++;
	}
	return count;
}

// API to support format in scanf, if you can't understand, use your own implementation!
int matchWhiteSpace(char *buffer, int size, int *count);
int str2Dec(int *dec, char *buffer, int size, int *count);
int str2Hex(int *hex, char *buffer, int size, int *count);
int str2Str2(char *string, int avail, char *buffer, int size, int *count);

int scanf(const char *format,...) {
    // TODO: implement scanf function, return the number of input parameters
	int i=0; // format index
	char buffer[MAX_BUFFER_SIZE];
	int count=0; // buffer index
	 int index=0; // parameter index
	 void *paraList=(void*)&format; // address of format in stack
	 //int state=0; // 0: legal character; 1: '%'; 2: illegal format
	 int* decimal=0;
	 int* hexadecimal=0;
	 char* string=0;
	 char* character=0;
	 int r=0;
	while(format[i]!=0){
        // TODO: support more format %s %d %x and so on
        	if(format[i]=='%'){
			i++;
		  switch(format[i]){
		  	
			case 'd':{
					 paraList+=4;
					 decimal=*(int**)paraList;
					 if(-1==str2Dec(decimal,buffer,MAX_BUFFER_SIZE,&count))
						 return -1;
					 index++;
					 //printf("d:%d,end d\n",*decimal);
					 break;
				}					 
			case '%':{
					 if(buffer[count]==0){
						do{
							r=syscall(SYS_READ, STD_IN, (uint32_t)buffer, (uint32_t)MAX_BUFFER_SIZE, 0, 0);
						}while(r == 0 || r == -1);
						count=0;
					}
					//if normal character match wrongly then return -1;
					if('%'!=buffer[count])
						return -1;	
					count++;
					break;
				}
			
			case 'x':{
					// printf("begin x\n");
					 paraList+=4;
					 hexadecimal=*(int**)paraList;
					 if(-1==str2Hex(hexadecimal,buffer,MAX_BUFFER_SIZE,&count))
						 return -1;
					 index++;
					 //printf("x:%x,end x\n",*hexadecimal);
					 break;
				}
			
			case 'c':{
					 //printf("begin c\n");
					 paraList+=4;
					 character=(char*)*(int*)paraList;
					if(buffer[count]==0){
						do{
							r=syscall(SYS_READ, STD_IN, (uint32_t)buffer, (uint32_t)MAX_BUFFER_SIZE, 0, 0);
						}while(r == 0 || r == -1);
						count=0;
					}
					
					*character=buffer[count];	
					count++;
					index++;
				 	//printf("c:%c,end c\n",*character);
				 	break;
				 }
			case 's':{
				 	//printf("begin s\n");
					 paraList+=4;
					 string=*(char**)paraList;
					int j=0;
					while(buffer[count]!='\0'&&buffer[count]!='\n'){
						if(buffer[count]==0){
							do{
								r=syscall(SYS_READ, STD_IN, (uint32_t)buffer, (uint32_t)MAX_BUFFER_SIZE, 0, 0);
							}while(r == 0 || r == -1);
							count=0;
						}

						string[j]=buffer[count];
						count++;
						j++;
					
					}
					string[j]='\0';
					count++;
					index++;
					//printf("s:%s,end s\n",string);
					break;

				 
				 
				 }	
			default:{
					if(format[i]>='0'&&format[i]<='9'){
						uint32_t k=format[i]-'0';
						i++;
						while(format[i]>='0'&&format[i]<='9'){
							k=10*k+format[i]-'0';
							i++;
						}
						if(format[i]!='s')
							return -1;
						else{
							paraList+=4;
							 string=*(char**)paraList;
							 //k plus one because taking '\0' into consideration
							if(-1==str2Str2(string,k+1,buffer,MAX_BUFFER_SIZE,&count))
								return -1;
							//printf("begin %ds\n",k); 
							/*int j=0;
							while(j<k){
								if(buffer[count]==0){
									do{
										r=syscall(SYS_READ, STD_IN, (uint32_t)buffer, (uint32_t)MAX_BUFFER_SIZE, 0, 0);
									}while(r == 0 || r == -1);
									count=0;
								}

								string[j]=buffer[count];
								count++;
								j++;
							}
							string[j]='\0';
							count++;
							index++;
							//printf("s:%s,end %ds\n",string,k);
							*/
							index++;
						}
						break;
					}
				        else return -1;	
				}
		 }
		}
		else if(format[i]==' '){
			matchWhiteSpace(buffer,MAX_BUFFER_SIZE,&count);
		}
		else{
			if(buffer[count]==0){
			do{
				r=syscall(SYS_READ, STD_IN, (uint32_t)buffer, (uint32_t)MAX_BUFFER_SIZE, 0, 0);
			}while(r == 0 || r == -1);
			count=0;
			}
			//if normal character match wrongly then return -1;
			//printf("buffer[count]:%c,format[i]:%c\n",buffer[count],format[i]);
			if(format[i]!=buffer[count])
				return -1;	
			count++;
		}
		if(count==MAX_BUFFER_SIZE) {
			count=0;
			buffer[count]=0;
		}
		i++;
	}
	
    return index;

    return 0;
}

int matchWhiteSpace(char *buffer, int size, int *count){
	int ret=0;
	while(1){
		if(buffer[*count]==0){
			do{
				ret=syscall(SYS_READ, STD_IN, (uint32_t)buffer, (uint32_t)size, 0, 0);
			}while(ret == 0 || ret == -1);
			(*count)=0;
		}
		if(buffer[*count]==' ' ||
		   buffer[*count]=='\t' ||
		   buffer[*count]=='\n'){
			(*count)++;
		}
		else
			return 0;
	}
}

int str2Dec(int *dec, char *buffer, int size, int *count) {
	int sign=0; // positive integer
	int state=0;
	int integer=0;
	int ret=0;
		if(buffer[*count]==0){
			//printf("tag1\n");
			do{
				ret=syscall(SYS_READ, STD_IN, (uint32_t)buffer, (uint32_t)size, 0, 0);
			}while(ret == 0 || ret == -1);
			(*count)=0;
		}

	while(1){
		if(state==0){
			if(buffer[*count]=='-'){
				state=1;
				sign=1;
				(*count)++;
			}
			else if(buffer[*count]>='0' &&
				buffer[*count]<='9'){
				state=2;
				integer=buffer[*count]-'0';
				(*count)++;
			}
			else if(buffer[*count]==' ' ||
				buffer[*count]=='\t' ||
				buffer[*count]=='\n'){
				state=0;
				(*count)++;
			}
			else
				return -1;
		}
		else if(state==1){
			if(buffer[*count]>='0' &&
			   buffer[*count]<='9'){
				state=2;
				integer=buffer[*count]-'0';
				(*count)++;
			}
			else
				return -1;
		}
		else if(state==2){
			if(buffer[*count]>='0' &&
			   buffer[*count]<='9'){
				state=2;
				integer*=10;
				integer+=buffer[*count]-'0';
				(*count)++;
			}
			else{
				if(sign==1)
					*dec=-integer;
				else
					*dec=integer;
				return 0;
			}
		}
		else
			return -1;
	}
	return 0;
}

int str2Hex(int *hex, char *buffer, int size, int *count) {
	int state=0;
	int integer=0;
	int ret=0;
	if(buffer[*count]==0){
			do{
				ret=syscall(SYS_READ, STD_IN, (uint32_t)buffer, (uint32_t)size, 0, 0);
			}while(ret == 0 || ret == -1);
			(*count)=0;
		}

	while(1){
		if(state==0){
			if(buffer[*count]=='0'){
				state=1;
				(*count)++;
			}
			else if(buffer[*count]==' ' ||
				buffer[*count]=='\t' ||
				buffer[*count]=='\n'){
				state=0;
				(*count)++;
			}
			else
				return -1;
		}
		else if(state==1){
			if(buffer[*count]=='x'){
				state=2;
				(*count)++;
			}
			else
				return -1;
		}
		else if(state==2){
			if(buffer[*count]>='0' && buffer[*count]<='9'){
				state=3;
				integer*=16;
				integer+=buffer[*count]-'0';
				(*count)++;
			}
			else if(buffer[*count]>='a' && buffer[*count]<='f'){
				state=3;
				integer*=16;
				integer+=buffer[*count]-'a'+10;
				(*count)++;
			}
			else if(buffer[*count]>='A' && buffer[*count]<='F'){
				state=3;
				integer*=16;
				integer+=buffer[*count]-'A'+10;
				(*count)++;
			}
			else
				return -1;
		}
		else if(state==3){
			if(buffer[*count]>='0' && buffer[*count]<='9'){
				state=3;
				integer*=16;
				integer+=buffer[*count]-'0';
				(*count)++;
			}
			else if(buffer[*count]>='a' && buffer[*count]<='f'){
				state=3;
				integer*=16;
				integer+=buffer[*count]-'a'+10;
				(*count)++;
			}
			else if(buffer[*count]>='A' && buffer[*count]<='F'){
				state=3;
				integer*=16;
				integer+=buffer[*count]-'A'+10;
				(*count)++;
			}
			else{
				/*if(buffer[*count]=='\0')
				printf("buffer[*count] is 0\n");
				else if(buffer[*count]=='\n')
				printf("buffer[*count] is n\n");*/
				*hex=integer;
				return 0;
			}
		}
		else
			return -1;
	}
	return 0;
}

int str2Str2(char *string, int avail, char *buffer, int size, int *count) {
	int i=0;
	int state=0;
	int ret=0;
	//printf("avail:%d\n",avail);
	while(i < avail-1){
		if(buffer[*count]==0){
			do{
				ret=syscall(SYS_READ, STD_IN, (uint32_t)buffer, (uint32_t)size, 0, 0);
			}while(ret == 0 || ret == -1);
			(*count)=0;
		}
		if(state==0){
			if(buffer[*count]==' ' ||
			   buffer[*count]=='\t' ||
			   buffer[*count]=='\n'){
				state=0;
				(*count)++;
			}
			else{
				state=1;
				string[i]=buffer[*count];
				i++;
				(*count)++;
			}
		}
		else if(state==1){
			if(buffer[*count]==' ' ||
			   buffer[*count]=='\t' ||
			   buffer[*count]=='\n'){
				string[i]=0;
				return 0;
			}
			else{
				state=1;
				string[i]=buffer[*count];
				i++;
				(*count)++;
			}
		}
		else
			return -1;
	}
	//printf("buffer[count]:%c\n",buffer[*count]);
	string[i]=0;
	return 0;
}
