#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<arpa/inet.h>
#include <fcntl.h>

#include <string.h>

#define MAX_LEN 5000

char buffer[MAX_LEN] = {0};
char temp[MAX_LEN] = {0};
int line = 0;
int finish_read= 0;

int isERR = 0;//if is 1, stop server after printout

int check = 0;
int ended = 0;

void clearbuff();
int check_pipe(char *message);//if 3, then all pipes were inputed, if not, continue read
void read_message(int new_socket);
int check_message(char *str);//0:true, 1:false, 2:error received close socket
int count_chars(char *str);//count number of characters
char last_char(char *str);//return last char


void clearbuff(){
  memset(buffer, 0, sizeof(buffer));
}

void cleartemp(){
  memset(temp, 0 , sizeof(temp));
}

int check_pipe(char* message){

  int count = 0;

  int i =0;
  
  while(message[i]){

    if(message[i] == '|'){
      count++;
      //printf("%d\n", i);
    }
    i++;
  }

  return count;
}

void read_message(int new_socket){
  
  int size = MAX_LEN;
  read(new_socket, buffer, size);

  strcpy(temp, buffer);

  clearbuff();
  
  check = check_message(temp);
  
  int num = 3;
  if(temp[0] == 'E'&& temp[1]=='R'&& temp[2]=='R'){

    num = 2;
  }
  
  int valread = 0;

  while(check_pipe(temp)<num && check==0){

    valread= read(new_socket, buffer, size);

    if(valread == 0){//
      printf("ERROR: M%dFT, missing pipe.\n", line);
      ended = 1;
      return;
    }
    
    strcat(temp, buffer);

    check = check_message(temp);

    if(check==1){

      return;
    }

    
    clearbuff();
  }

}


int count_chars(char *str){
  int count = 0;
  int i =0;
  while(str[i]){
    count++;
    i++;
  }
  return count;
}


char last_char(char *str){
  int i=0;
  while(str[i]){
    if(str[i+1]) i++;
    else break;
  }
  return str[i];
}


int check_message(char *original){

  char str[MAX_LEN] = {0};
  strcpy(str, original);
  
  char line_c[5];
  sprintf(line_c, "%d", line);

  
  int count_pipe = 0;
  int type = 0;//REG:1, ERR:2

  int expect = 0;//expect number of chars
  int actual = 0;//actual number of chars
  
  int i = 0;


  while(str[i]){
    
    if(i==0){
      if(strlen(str)<=4)break; //check for message type
      
      if(str[0]=='R' && str[1]=='E' && str[2]=='G'){
	type = 1;
	i += 3;//stop at '|'

      }
      
      else if(str[0]=='E' && str[1]=='R' && str[2]=='R'){
	type = 2;
	isERR = 1;
	//limit = 2;
	i+=3;
      }
      else{//wrong message type
	strcpy(buffer,"ERR|M");
	strcat(buffer,line_c);
	strcat(buffer,"FT|");
	check = 1;
	return check;
      }
    }

    else if(count_pipe==1 && type==1){//check word length for REF

      int j = 0;
      char num[MAX_LEN] = {0};
      
      while(str[i] != '|'){

	if(isalpha(str[i]) ){//in section for length, there is a letter, format error
	  strcpy(buffer,"ERR|M");
	  strcat(buffer,line_c);
	  strcat(buffer,"FT|");
	  check=1;
	  return check;
	}
	//input digit into num
	num[j] = str[i];
	i++;
	j++;
      }

      if(!num[0]){
	strcpy(buffer,"ERR|M");
	strcat(buffer,line_c);
	strcat(buffer,"FT|");
	check=1;
	return check;
      }
      expect = atoi(num);
     
      memset(num, 0 , strlen(num));
    }

    else if(count_pipe==1 && type==2){//check content for ERR
      
      if(str[i]=='|'){
	strcpy(buffer,"Warning: ERR received is incorrectly formatted.");

	check=1;
	return check;
      }
      
      else if(str[i]=='M'){
	i++;//should be line number
	if(str[i] == line_c[0]){//the line char in error should be the same as line_c
	  i++;
	  if((str[i]=='F' && str[i+1]=='T') ||
	     (str[i]=='L' && str[i+1]=='N')||
	     (str[i]=='C' && str[i+1]=='T')){

	    i+=2;
	    
	  }//str[i] should be '|'

	  else{//content error
	    strcpy(buffer,"Warning: ERR received is incorrectly formatted.");

	    check = 1;
	    return check;
	  }

	  if(str[i] != '|'){//exceeds expected length for ERR
	    strcpy(buffer,"Warning: ERR received is incorrectly formatted.");
	    
	    check = 1;
	    return check;
	  }
	}
	else{//content error for being wrong char as line_c
	  strcpy(buffer,"Warning: ERR received is incorrectly formatted.");
	  
	  check=1;
	  return check;
	}
      }
      else{
	strcpy(buffer,"Warning: ERR received is incorrectly formatted.");
	
	check = 1;
	return 1;
      }
      
    }
    //after 2nd pipe, it is message content
    else if(type==1 && count_pipe==2){

      int j = 0;
      char content[MAX_LEN]={0};
      
      while(str[i]!='|'){

	if(!str[i]) break;//if null breka
		
	content[j] = str[i];
	actual++;
	
	if(actual > expect){

	  strcpy(buffer,"ERR|M");
	  strcat(buffer,line_c);
	  strcat(buffer,"LN|");
	  check = 1;
	  return check;
	}
	
	i++;
	j++;
      }//input of chars complete

      if(!content[0]){//for empty content
	strcpy(buffer,"ERR|M");
	strcat(buffer,line_c);
	strcat(buffer,"FT|");
	check = 1;
	return check;
      }
      
      //line 0, 1, 3 have conditions in message
      if(actual != expect && str[i]=='|'){
	//if actual less than expect
	printf("expect: %d, actual: %d\n", expect, actual);
	strcpy(buffer,"ERR|M");
	strcat(buffer,line_c);
	strcat(buffer,"LN|");
	check=1;
	return check;
      }
      
      if(line == 0){
	if(strcmp(content, "Knock, knock.")!=0){
	  strcpy(buffer,"ERR|M");
	  strcat(buffer,line_c);
	  strcat(buffer,"CT|");
	  check=1;
	  return check;
	}
      }
      else if(line==1 && strlen(content)==12){
	if(strcmp(content, "Who's there?")!= 0){
	  strcpy(buffer,"ERR|M");
	  strcat(buffer,line_c);
	  strcat(buffer,"CT|");
	  check=1;
	  return check;
	}
      }
      else if(line==3 && str[i]=='|'){
	int len = actual-1;//before |

	if(content[len]=='?' &&
	   content[len-1]=='o' &&
	   content[len-2]=='h' &&
	   content[len-3]=='w' &&
	   content[len-4]==' ' &&
	   content[len-5]== ','){//should end with ", who?"
	  check=0;
	}
	else{//if ", who?" not found
	  strcpy(buffer,"ERR|M");
	  strcat(buffer,line_c);
	  strcat(buffer,"CT|");
	  check=1;
	  return check;
	}
      }
      else if(line ==2 || line==4 || line==5){
	//include 2,4,5, check for punct
	int last = actual - 1;
	if(content[last]=='.' || content[last]=='?' || content[last]=='!'){
	  
	  check=0;
	}
	else{//no end sentence punctuation
	  strcpy(buffer,"ERR|M");
	  strcat(buffer,line_c);
	  strcat(buffer,"CT|");
	  check=1;
	  return check;
	}
      }
    }

    if(str[i] == '|'){//check pipe

      count_pipe++;
      i++;
      if((type==1 && count_pipe==3 && last_char(str)!='|' )||
	 (type==2 && count_pipe == 2 && last_char(str)!='|') ){
	//if exceed |
	if(type==1){
	  strcpy(buffer,"ERR|M");
	  strcat(buffer,line_c);
	  strcat(buffer,"FT|");
	  check =1;
	  return check;
	}
	else if(type==2){
	  strcpy(buffer,"Warning: ERR received is incorrectly formatted.");
	}
	
      }
      
    }

  }


  return check;
}



int main(int argc, char *argv[]){

  int server_fd, new_socket;//, valread;
  struct sockaddr_in socket_address;
    
  int opt = 1;
  int addrlen = sizeof(socket_address);
    
  char *knock  = "REG|13|Knock, knock.|";
  char *quote1 = "REG|4|Bad.|";
  char *finish = "REG|16|A very bad joke.|";

  //int check = 0;
  
  int port = atoi(argv[1]);
    
  if(port<5000 || port >65535){
    printf("ERROR: port is out of bounds\n");
    return EXIT_FAILURE;
  }
    
  if((server_fd = socket(AF_INET, SOCK_STREAM, 0))==0){
    printf("Socket creation failed\n");
    return EXIT_FAILURE;
  }

  //attach socket to PORT
  if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt) )){
    printf("Set Socket Opt failed\n");
    return EXIT_FAILURE;
  }
    

  memset(&socket_address, 0, addrlen);//fill with zero
  
  socket_address.sin_family = AF_INET;
  socket_address.sin_addr.s_addr = INADDR_ANY;//attach any available prot
  
  socket_address.sin_port = htons( port ); //read the port
  
  //attach socket to PORT
  if(bind(server_fd, (struct sockaddr *) &socket_address, sizeof(socket_address))<0){
    printf("Bind error\n");
    return EXIT_FAILURE;
  }
    
  if(listen(server_fd, 3) < 0){
    printf("listen failed\n");
    return EXIT_FAILURE;
  }
    
  //to not close server, keep it opened
  while(1){
      //accpet message from client
    new_socket = accept(server_fd, (struct sockaddr *) &socket_address, (socklen_t *) &addrlen);

      
    if(new_socket < 0){
      printf("Acception error");
      return EXIT_FAILURE;
    }
      
        
    //LINE0, send knock knock
     
    check = check_message(knock);
    if(check==1){
      write(new_socket, buffer, strlen(buffer));
      return 0;
    }
    write(new_socket, knock, strlen(knock));
    
    clearbuff();
    
    //LINE1,read and print respond1
    line++;
    
    read_message(new_socket);
    
    printf("%s\n",temp);
    if(ended==1)return 0;
    if (check==1){
      write(new_socket, buffer, strlen(buffer));
      
      return 0;
    }
    if(isERR==1){
      return 0;
    }
    cleartemp();
    
      
    //LINE2,send quote1
    line++;
    check = check_message(quote1);
    if(check==1){
      write(new_socket, buffer, strlen(buffer));
      return 0;
    }
    write(new_socket, quote1, strlen(quote1)); 
    clearbuff();
          
    //LINE3, read respond2
    line++;
    
    read_message(new_socket);
    
    printf("%s\n", temp);
    if(ended==1)return 0; 
    if(check==1){
      write(new_socket, buffer, strlen(buffer));
      //printf("%d. %s\n", line, buffer);
      return 0;
    }
    if(isERR==1) return 0;
    cleartemp();
    
    
    //LINE4, send quote2
    line++;
      
    //write(new_socket, finish, strlen(finish));
    check = check_message(finish);
    if(check == 1){
      write(new_socket, buffer, strlen(buffer));
      return 0;
    }
    write(new_socket, finish, strlen(finish));
    clearbuff();
    
    //LINE5,read respond3
    line++;
    read_message(new_socket);
    
    printf("%s\n", temp);
    if(ended==1)return 0; 
    
    if(check == 1){
      write(new_socket, buffer, strlen(buffer));
      
      return 0;
    }
    if(isERR==1)return 0;
    cleartemp();      
    
      
    line = 0;
    //printf("Line: %d", line);
    
    close(new_socket);
  
  }
  
}
