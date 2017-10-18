#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>


//list command test

#define PATH "../Server/mailspool/"
#define USER "if16b502"
#define NUMBER 2
int main() {

    //DEL
    //if16b502
    //1
    //    OK\n

    //delProtocol


    char filepath[256] = "";
    struct dirent **namelist;

    strcpy(filepath, PATH); //pfad zum spool
    strcat(filepath, USER); //angegebener user
    strcat(filepath, "/"); //slash

    printf("%s\n", filepath);

    int n = scandir(filepath, &namelist, NULL, alphasort); //sortiere alphabetisch

    printf("%i\n", n-2); //anzahl elemente ohne . und ..

    if(n < 0 || NUMBER < 0 || NUMBER > (n-2)) { //fehler wenn ordner nicht existiert oder nummer negativ oder nummer größer als anzahl nachrichten
        printf("ERR\n");
    } else {
        for(int i = 2; i < n; i++) { //ersten 2 elemente überspringen ( . und .. )

            //printf("%s\n", namelist[i]->d_name);

            if((i-1) == NUMBER) { //menschen zählen 1, 2, 3, 4 und nicht 0, 1, 2 ,3
            //remove file

                strcat(filepath, namelist[i]->d_name);

                printf("Removing file: %s\n", namelist[i]->d_name);
                printf("filepath: %s\n", filepath);
                if (remove(filepath) == 0) {
                    //success
                    printf("OK\n");
                } else {
                    //error
                    printf("ERR\n");
                }
            }
            free(namelist[i]);
        }
        free(namelist);
    }
}
