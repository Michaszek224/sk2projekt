#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

int main(int argc, char* argv[]){
	cout << "test0" << endl;
	if (argc < 2 || argc > 2) {
        cerr << "Użycie: " << argv[0] << " <port>\n";
        return 1;
    }
	int server_port = atoi(argv[1]);
	const char* server_ip = "127.0.0.1";
	int client_socket = socket(AF_INET,SOCK_STREAM,0);
	cout << "test1" << endl;
	if(client_socket == -1){
		cerr << "nie mozna utworzyc socketu\n";
		return 1;
	}
	cout << "test11" <<endl;	
	sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(server_port);

	// Ustawienie adresu IP serwera
	if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) {
		cerr << "Nieprawidłowy adres IP" << endl;
		close(client_socket);
		return 1;
	}
	
	cout << "test12" <<endl;
	if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
    	perror("Nie mozna polaczyc się z serwerem\n");
    	close(client_socket);
    	return 1;
	}

	cout<<"polaczono z serverem" << endl;
	cout << "test2" <<endl;	
	//polaczenie do servera glownego
	if(server_port == 8080)
	{
		cout << "test3" <<endl;
		char buffer[1024] = {0};
		ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
		if (bytes_received > 0) {
			buffer[bytes_received] = '\0';  // Dodaje zakończenie tekstowe
			cout << "Otrzymano odpowiedź: " << buffer << endl;
		}

// Pozwól na dodatkową interakcję (np. oczekiwanie na więcej komunikatów od serwera)


	
		//wybranie czy jestes graczem czy hostem
		char graczCzyHost[1024];
		cout << "Wybierz rolę: 'host' lub 'gracz': ";

		int ileWyslanych = read(0,graczCzyHost,sizeof(graczCzyHost)-1);
	
		if (send(client_socket,graczCzyHost,ileWyslanych,0) == -1){
			cerr<<"nie mozna wyslac danych";
		} else {
			cout <<"wiadomosc wyslana\n";
		}
		
		//kiedy jestes hostem
		if(graczCzyHost[0] == 'h' && graczCzyHost[1] == 'o' && graczCzyHost[2] == 's' && graczCzyHost[3] == 't'){
			//przekazuje port i kod dostepu hosta
			buffer[1024] = {0};
			bytes_received = recv(client_socket,buffer,sizeof(buffer) -1,0);
			if(bytes_received > 0){
				cout << buffer;
			}
		}
		//kiedy jestes graczem
		else{
			//podaje kod
			char kodGry[1024];
			ileWyslanych = read(0,kodGry,sizeof(kodGry)-1);
			send(client_socket,kodGry,ileWyslanych,0);
			
			//przekazuje port na ktory ma sie polaczyc
			buffer[1024] = {0};
			bytes_received = recv(client_socket,buffer,sizeof(buffer) -1,0);
			if(bytes_received > 0){
				cout << buffer;
			}
		}
	
		close(client_socket);
		return 0;
	}
	else
	{
		char buffer[1024] = {0};
		ssize_t bytes_received = recv(client_socket,buffer,sizeof(buffer) -1,0);
		
		//"nie ma gry"
		
		if(buffer[0]=='n' && buffer[1] == 'i' && buffer[2]=='e'){
		
			//host podaje kod hosta
			
			char kodHosta[1024];
	
			int ileWyslanych = read(0,kodHosta,sizeof(kodHosta)-1);
	
			if (send(client_socket,kodHosta,ileWyslanych,0) == -1){
				cerr<<"nie mozna wyslac danych";
			} else {
				cout <<"wiadomosc wyslana\n";
			}
			
			//wybiera quiz
			
			//czy chcesz gotowy quiz?
			char odpowiedz[3];
			
			ileWyslanych = read(0,odpowiedz,sizeof(odpowiedz)-1);
			
			if (send(client_socket,odpowiedz,ileWyslanych,0) == -1){
				cerr<<"nie mozna wyslac danych";
			}
			
			//jezeli tak
			if(odpowiedz[0] == 't' && odpowiedz[1] == 'a' && odpowiedz[2] == 'k'){
			
				char ktoraOpcja[2];
				ileWyslanych = read(0,odpowiedz,sizeof(odpowiedz)-1);
			
				if (send(client_socket,odpowiedz,ileWyslanych,0) == -1){
					cerr<<"nie mozna wyslac danych";
				}
			}
			//jezeli nie
			else if(odpowiedz[0] == 'n' && odpowiedz[1] == 'i' && odpowiedz[2] == 'e'){
			
				//pytanie ile pytan maksymalnie 99
				
				char ilePytan[2];
				ileWyslanych = read(0,ilePytan,sizeof(ilePytan)-1);
			
				if (send(client_socket,ilePytan,ileWyslanych,0) == -1){
					cerr<<"nie mozna wyslac danych";
				}
				
				for (int i = 0; i < stoi(ilePytan); i++){
				
					//napisz pytanie
					char pytanie[1024];
					ileWyslanych = read(0,pytanie,sizeof(pytanie)-1);
			
					if (send(client_socket,pytanie,ileWyslanych,0) == -1){
						cerr<<"nie mozna wyslac danych";
					}
					
					//napisz odpowiedzi
					for(int j = 0; j < 4; j++){
						
						char odpowiedzi[1024];
						ileWyslanych = read(0,odpowiedzi,sizeof(odpowiedzi)-1);
			
						if (send(client_socket,odpowiedzi,ileWyslanych,0) == -1){
							cerr<<"nie mozna wyslac danych";
						}
					}
					
					//ktora odpowiedz jest prawidlowa?
					
					char prawidlowaOdpowiedz[2];
					ileWyslanych = read(0,prawidlowaOdpowiedz,sizeof(prawidlowaOdpowiedz)-1);
			
						if (send(client_socket,prawidlowaOdpowiedz,ileWyslanych,0) == -1){
							cerr<<"nie mozna wyslac danych";
						}
				}
			}
			//nieprawidlowa wiadomosc
			else{
				cout<<"nieprawidlowa odpowiedz\n";
				close(client_socket);
				return 0;
			}
			
			
			
			while(1){
			
				//przeprowadzana jest gra host dostaje tylko staty
				
				buffer[1024] = {0};
				bytes_received = recv(client_socket,buffer,sizeof(buffer) -1,0);
				if(buffer[0]=='k' && buffer[1] == 'o' && buffer[2]=='n'){
					break;
				}
				else if(bytes_received > 0){
					cout << buffer;
				}
			}
		}
		//jest gra
		else{
			//gracz
			while(1){
				//dostaje pytanie i musi odpowiedziec
				buffer[1024] = {0};
				bytes_received = recv(client_socket,buffer,sizeof(buffer) -1,0);
				cout<<buffer;
				char odpowiedz[2];
				read(0,odpowiedz,sizeof(odpowiedz)-1);
				if (send(client_socket,odpowiedz,1,0) == -1){
					cerr<<"nie mozna wyslac danych";
				}
			}
		}
	
		close(client_socket);
		return 0;
	}
	

}
