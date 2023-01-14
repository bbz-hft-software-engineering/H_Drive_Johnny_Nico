// SES6-02b-TCP-C.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <regex>

#pragma comment(lib,"ws2_32.lib")
using namespace std;

// Funktion Prüft ob die eingegebene IP Adresse logisch korekt ist.
bool isValidIP(string IP)
{
    regex ipRegex("(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])");
    return regex_match(IP, ipRegex);
}

// Funktion welche den Ping Befehl Ausführt // True = OK / False = Not OK
bool ping_ip(string ipAddress)
{
    string command = "ping " + ipAddress + " -n 2";
    int ret = system(command.c_str());
    if (ret == 0)
        return true;
    else
        return false;
}

// Das Empfangene Telegram vom Motor wird als Char an diese Funktion übergeben, die Funktion gibt nur den Wert Speed aus dem Telegramm als String zurück.
string getSpeed(string driveDebug)
{
    string speed;
    size_t begin = driveDebug.find("Speed=\"");
    if (begin != string::npos)
    {
        begin += 7;
        size_t end = driveDebug.find("\"", begin);
        speed = driveDebug.substr(begin, end - begin);
    }
    return speed;
}




int main(void) {

    int PORT = 1000; //Server Port
    string SERVERADDRESS; //server adresse
    const int MESSAGELENGTH = 450; //max Länger der Nachricht
    SOCKET tcpSocket;  //Socket Objekt erstellen, benötigt für socket()
    struct sockaddr_in serverAdress, clientAddress; //Structs in denen wir die Daten zum Server und Client Speichern
    int slen, recv_len; //Variablen gebraucht in sendto() und recvfrom()
    const char* message;   //in message speichern wir die versendeten Daten
    WSADATA wsaData;   //WSADATA Objekt, benötigt für WSAStartup()
    slen = sizeof(clientAddress); //Länge der Client Adresse, gebruacht in sendto()
    string IP; // IP von der Usereingabe, plausibilitätsprüfung
    int Port; // Port von der Usereingabe,plausibilitätsprüfung
    string Mleft = "<control pos = \"100000\" speed = \"5000\" torque = <\"-3000\" mode = \"128\" acc = \"200000\" decc = \"1\" / >";  // Telegramm Linkslauf 
    string Mright = "<control pos = \"100000\" speed = \"5000\" torque = <\"2000\" mode = \"128\" acc = \"200000\" decc = \"1\" / >";   // Telegramm Rechtslauf
    string Mstop = "<control pos = \"100000\" speed = \"5000\" torque = <\"2000\" mode = \"0\" acc = \"200000\" decc = \"1\" / >";      // Telegramm Stopp
    bool IPinput = false; //Zeigt an ob die IP und der Port registriert wurden
    bool IPcheck = false;   // Zeigt an ob die IP adresse und Port erfolgreich überprüft wurde. Der TCP Socket kann ab nun starten.
    bool triggerkey = false;    //True wenn eine Taste gedrückt wird (W oder S)
    bool motorrotation = false; // true wenn der Motor in Bewegung ist
    char feedbackmessage[MESSAGELENGTH]; // Empfangenes Telegramm
    int zaeler = 0; // Verzögerung um Daten empfangene Daten anzuzeigen
    



    while (1) {

        // IP und Port Einlesen, Begrüssung 
        if (IPinput == false) 
        {
            cout << "###################################################" << endl;
            cout << "#_____________Motor Controler over IP ____________#" << endl;
            cout << "###################################################" << endl;

            while (IPcheck == false)
            {
                cout << "Bitte geben Sie die IP-Adresse ihres Motors ein" << endl;
                cin >> IP;
                cout << "Geben Sie den Port des Motors ein" << endl;
                cin >> Port;

                // überprüfung der IP Adresse (kontrolliert Format und führt Ping aus), dafür wird die eingegebene IP Adresse an die Funktion übergeben

                if (isValidIP(IP) && ping_ip(IP)==true)
                {
                    IPinput = true;
                    IPcheck = true;
                    cout << endl;
                    cout << "Teilnehmer ist im Netzwerk Aktiv" << endl;;
                }
                else
                {
                    cout << IP << " ist eine Ungueltige IP Adresse." << endl << endl;
                    
                }
            }
            // Serveradresse und Port werden an den TCP Socket übergeben
            SERVERADDRESS = IP;
            PORT = Port;

            // Initialise Winsock
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                cout << "WSAStartup fehlgeschlagen: " << WSAGetLastError() << endl;
                system("pause");
                return 1;
            }

            // Create tcp socket
            if ((tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
            {
                cout << "Socket erstellen fehlgeschlagen: " << WSAGetLastError() << endl;
                system("pause");
                return 1;
            }

            memset((char*)&serverAdress, 0, sizeof(serverAdress)); //Reserviert Platz im Memory für die Daten


            //Block 1
            serverAdress.sin_family = AF_INET;
            serverAdress.sin_port = htons(PORT);
            serverAdress.sin_addr.S_un.S_addr = inet_addr(SERVERADDRESS.c_str());

            //connect + fehler
            if (connect(tcpSocket, (struct sockaddr*)&serverAdress, slen) == INVALID_SOCKET)
            {
                cout << "Verbindung zu " << SERVERADDRESS.c_str() << " fehlgeschlagen: " << WSAGetLastError() << endl;
                system("pause");
                return 1;
            }

            // Steuerungsanleitung / Rückmeldung Verbindung I.O.
            cout << "Verbunden mit Server " << SERVERADDRESS.c_str() << endl;
            cout << "----> Motor Rechts = W " << endl;
            cout << "<---- Motor Links = S " << endl;
            cout << "Programm Beenden = A" << endl;

        }

        // Hier werden die Telegramme gesendet. Pro Tastenflanke wird 1 Telegramm gesendt

        // Rechtslauf
        if (GetAsyncKeyState('S') & 0x8000) {   
            if (triggerkey == false) {
                message = Mright.c_str();
                motorrotation = true;
                triggerkey = true;
                send(tcpSocket, message, strlen(message), 0);
            }
        }
        // Linkslauf
        else if (GetAsyncKeyState('W') & 0x8000) { 
            if (triggerkey == false) {
                message = Mleft.c_str();
                motorrotation = true;
                triggerkey = true;
                send(tcpSocket, message, strlen(message), 0);
            }
        }
        // Verbindung beenden
        else if (GetAsyncKeyState('A') & 0x8000) {  
            closesocket(tcpSocket);
            WSACleanup();
            system("pause");
            return 0;
        }
        //Motor Stoppen wenn keine Taste gedrückt wird
        else if (motorrotation == true) {   
            message = Mstop.c_str();
            triggerkey = false;
            motorrotation = false;
            send(tcpSocket, message, strlen(message), 0);


        }
       


        //Daten empfangen
        memset(feedbackmessage, '\0', MESSAGELENGTH);
        // Fehlerbedinung
        if ((recv_len = recv(tcpSocket,feedbackmessage, MESSAGELENGTH, 0)) == SOCKET_ERROR)
        {
            cout << "Daten empfangen fehlgeschlagen: " << WSAGetLastError() << endl;
            system("pause");
            return 1;
        }
        // Sobald der Motor in Bewegung ist, wird der Geschwindigkeitswer alle 1000 Zyklen ausgegeben 
        else if (zaeler > 1000 && motorrotation == true ) {

            cout << "Speed = " << getSpeed(feedbackmessage) << endl;
            zaeler = 0;
        }
        zaeler++;
        
    }


}

