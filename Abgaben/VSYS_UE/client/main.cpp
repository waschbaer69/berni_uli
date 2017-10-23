#include "Mclient.h"

int main (int argc, char **argv) 
{
	//Client Objekt wird erstellt
    MClient client(argc, argv);

    // Auswahlmenu für den Benutzer
    while(1)
    {
    	int choice = 0;
    	string recvmsg = "";
    	cout<<"SEND (1)"<<endl;
    	cout<<"LIST (2)"<<endl;
    	cout<<"READ (3)"<<endl;
    	cout<<"DEL  (4)"<<endl;
    	cout<<"QUIT (5)"<<endl;
    	cout<<"---------";
    	cout<<"Your wish is my command: ";
    	cin >> choice;
    	cout<<"---------";
		
    	/* 	Der Client sendet dem Server bei der Auswahl einer Funktion den
			Befehl mit. Z.b. Aufrufen der Methode list(string username) sendet
			der Client folgendes "LIST: Username". Der Server trennt Befehl von
			der Information und weiß dann genau was er zutun hat.
    	*/

    	/*
    	*	Falls der Benutzer versucht einen string als Input zu übergeben,
    	*	muss cin gecleart werden, anonsten Triggerschleife.
    	*/
    	if (!cin)
    	{
		    cout << "Input is not a number." << endl;
		    cin.clear();
			cin.ignore(10000,'\n');
		}

    	switch(choice)
    	{
    	/**********************			SEND 		**********************/
    	case 1:
    		{
    			/*
					Dem Benutzer wird erst nach falscher Eingabe darauf 
					hingewiesen, wie viele Zeichen er für jedes
					E-Mail Feld zu Verfügung hat.
					- Sender (max. 8)
					- Empf (max. 8)
					- Betreff (max. 80)
					- Nachricht (unbegrenzt und wird mit \nEND beendet)
    			*/
	    		string msg = "SEND:";
				string temp = "";
				cout << endl << "Sender: ";
				while(cin >> temp)
				{
					if(temp.length() < 9) break;
					else 
					{
						cout << "max. 8 Zeichen" << endl;
						cout << "Sender: ";
					}
				}
				msg += temp;
				cout << "Empfaenger: ";
				while(cin >> temp)
				{
					if(temp.length() < 9) break;
					else 
					{
						cout << "max. 8 Zeichen" << endl;
						cout << "Empfaenger: ";
					}
				}
				msg += "\n"+temp;
				cout << "Betreff: ";
				cin.ignore();
				while(getline(cin, temp))
				{
					if(temp.length() < 81) break;
					else 
					{
						cout << "max. 80 Zeichen" << endl;
						cout << "Betreff: ";
					}
				}
				msg += "\n"+temp;
				cout << "Nachricht: (Stop with '\\" << "nEND')" << endl;
				
				while (getline(cin, temp))
				{
	    			if(temp == "END") break;
	    			else
	    			{
	    				msg += "\n" + temp;
	    			}
				}

				//Felder wurden ausgefüllt -> Aufrufen der sendmsg() Methode
				client.sendmsg(msg);

				//Client empfängt OK / ERR
				recvmsg = client.recvmsg();
				cout << "Server-Antwort: "<< recvmsg << endl;
			}
			break;
		/**********************			LIST 		**********************/
		case 2:
			{
				string temp = "";
				string msg = "LIST:";
				cout << "USERNAME: ";
				cin >> temp;
				msg += temp;

				//Felder wurden ausgefüllt -> Aufrufen der listmsg() Methode
				client.listmsg(msg);

				//Nachrichtenempfang (Betreff)
				recvmsg = client.recvmsg();
			    cout << recvmsg << endl;
		    }
		    break;
		/**********************			READ 		**********************/
		case 3:
			{
				string temp = "";
				string msg = "READ:";

				cout << "USERNAME: ";
				cin >> temp;
				msg += temp;
				cout << "MESSAGE-ID: ";
				cin >> temp;
				msg += "\n"+temp;

				//Felder wurden ausgefüllt -> Aufrufen der readmsg() Methode
				client.readmsg(msg);

				//Nachrichtenempfang
				recvmsg = client.recvmsg(); 
				if(recvmsg == "ERR\nUser not found\n" || recvmsg == "ERR\n") cout << recvmsg << endl;
				else
				{
					Mmessage emailobject(recvmsg);
					cout << "-------------------------------------------" << endl;
				    cout << "| Absender: " << emailobject.get_sender() << endl;
				    cout << "| Empfaenger: " << emailobject.get_empf() << endl;
				    cout << "| Betreff: " << emailobject.get_betreff() << endl;
				    cout << "| Nachricht: " << endl << emailobject.get_nachricht();
				    cout << "-------------------------------------------" << endl;	
				}
			}
			break;
		/**********************			DELETE 		**********************/
		case 4:
			{
				string temp = "";
				string msg = "DELE:";

				cout << "USERNAME: ";
				cin >> temp;
				msg += temp;
				cout << "MESSAGE-ID: ";
				cin >> temp;
				msg += "\n"+temp;
				client.delmsg(msg);

				//NachrichtenEmpfang
				recvmsg = client.recvmsg();
				//OK - ERR?
				cout << "Server-Antwort: "<< recvmsg << endl;
			}
			break;
		/* QUIT */
		case 5:
			return 0;
		/* Benutzer gibt etwas Falsches ein */
		default: 
			cout<<"This choice is not given."<<endl;
		}
		client.clearbuffer();
	} 
	client.closeconnection();
	return EXIT_SUCCESS;
}




