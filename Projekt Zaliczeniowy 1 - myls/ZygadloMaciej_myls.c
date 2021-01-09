#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<sys/stat.h>
#include<unistd.h>
#include<sys/types.h>
#include<limits.h>
#include<dirent.h>
#include<grp.h>
#include<pwd.h>
#include<errno.h>
#include<stdlib.h>
#include<stdbool.h>

#define MAXROWLEN  80

char PATH[PATH_MAX+1];

int g_leave_len = MAXROWLEN;
int g_maxlen;

void my_err(const char* err_string,int line);
void display_dir(char* path);
void my_err(const char* err_string,int line){
    fprintf(stderr,"myls error : ");
    perror(err_string);
    exit(1);
}
char* date_format(char* str, time_t val){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    struct tm tmm = *localtime(&val);
    // „Dla plików z czasem, który jest wysunięty więcej niż 6 miesięcy w przeszłość
    // lub 1 godzinę w przyszłość, timestamp zawiera określenie roku zamiast godziny.”
   
    long int t_6m = 15778800000; // 6 miesiecy jako milisekundy
    long int t_1h = -3600000; // 1 godzina jako milisekundy

    time_t t_sys = mktime(&tm);
    time_t t_file = mktime(&tmm);
    double diffSecs = difftime(t_sys, t_file);
    if (diffSecs > t_6m || diffSecs < t_1h){
        // Starsze niz 6 miesiecy
        strftime(str, 36, "%Y-%m-%d", localtime(&val));
    } else {
        strftime(str, 36, "%m-%d %H-%M", localtime(&val));
    }

    return str;
}
void  display_attribute(char* name) { 
    struct stat buf;
    char buff_time[32];
    struct passwd* psd;  //Receive the user name of the file owner from this structure
    struct group* grp;   //Get group name
    if(lstat(name,&buf)==-1) {
        my_err("stat",__LINE__);
    }
    if(S_ISLNK(buf.st_mode))
        printf("l");
    else 
        if(S_ISREG(buf.st_mode))
        printf("-");
    else 
        if(S_ISDIR(buf.st_mode))
        printf("d");
    else 
        if(S_ISCHR(buf.st_mode))
        printf("c");
    else 
        if(S_ISBLK(buf.st_mode))
        printf("b");
    else 
        if(S_ISFIFO(buf.st_mode))
        printf("f");
    else 
        if(S_ISSOCK(buf.st_mode))
        printf("s");
    //Get Print File Owner Rights
    if(buf.st_mode&S_IRUSR)
        printf("r");
    else
        printf("-");
    if(buf.st_mode&S_IWUSR)
        printf("w");
    else
        printf("-");
    if(buf.st_mode&S_IXUSR)
        printf("x");
    else
        printf("-");

    //All group permissions
    if(buf.st_mode&S_IRGRP)
        printf("r");
    else
        printf("-");
    if(buf.st_mode&S_IWGRP)
        printf("w");
    else
        printf("-");
    if(buf.st_mode&S_IXGRP)
        printf("x");
    else
        printf("-");

    //Other people's rights
    if(buf.st_mode&S_IROTH)
        printf("r");
    else
        printf("-");
    if(buf.st_mode&S_IWOTH)
        printf("w");
    else
        printf("-");
    if(buf.st_mode&S_IXOTH)
        printf("x");
    else
        printf("-");

    printf("  ");
    psd=getpwuid(buf.st_uid);
    grp=getgrgid(buf.st_gid);
    printf("%2lu ",buf.st_nlink);
    printf("%-8s ",psd->pw_name);
    printf("%-8s ",grp->gr_name);

    printf("%5lu",buf.st_size);

    printf("  %s  ", date_format(buff_time, buf.st_mtime));
    printf("%s ",name);
    char pt[1024];
    ssize_t len = readlink(name, pt, 1023);
    pt[len]='\0';
    if(S_ISLNK(buf.st_mode))
        printf(" -> %s", pt);
    printf("\n");
}
void display(char **name ,int count){
    int i;
    for(i=0;i<count;i++){
        display_attribute(name[i]);
    }
}
void display_dir(char* path){
    DIR* dir;
    struct dirent* ptr;
    int count=0;

    dir = opendir(path);
    if(dir==NULL)
        my_err("opendir",__LINE__);
    g_maxlen=0;
    while((ptr=readdir(dir))!=NULL)
    {
        if(g_maxlen<strlen(ptr->d_name))
            g_maxlen=strlen(ptr->d_name);
        count++;
    }
    closedir(dir);
    char **filenames=(char**)malloc(sizeof(char*)*count),temp[PATH_MAX+1];
    int a; 
    for(a=0;a<count;a++){
        filenames[a]=(char*)malloc(sizeof(char)*PATH_MAX+1);
    }

    int i,j;
    dir=opendir(path);
    for(i=0;i<count;i++)
    {
        ptr=readdir(dir);
        if(ptr==NULL)
        {
            my_err("readdir",__LINE__);
        }
        strcpy(filenames[i],ptr->d_name);
    }
    closedir(dir);
    for(i=0;i<count-1;i++) {
        for(j=0;j<count-1-i;j++) {
            if(strcmp(filenames[j],filenames[j+1])>0) {       
                strcpy(temp,filenames[j]);
                strcpy(filenames[j],filenames[j+1]);
                strcpy(filenames[j+1],temp);
            }
        }
    }

    if(chdir(path)<0){
        my_err("chdir",__LINE__);
    }

    display(filenames,count);   
    printf("\n");
}
char* formatdate(char* str, time_t val){
        strftime(str, 36, "%d %B %Y roku o %H:%M:%S", localtime(&val));
        return str;
}
void  display_attribute_detailed(char* name) {
    char buff_time[32];
    struct stat buf;
    if(lstat(name,&buf)==-1) {
        my_err("stat",__LINE__);
    }
    printf("Informacje o %s:\n", name);
    printf("Typ Pliku :%c", '\0');
    if(S_ISLNK(buf.st_mode)) {
        printf("link symboliczny\n");
    } else if(S_ISREG(buf.st_mode)){
        printf("zwykly plik\n");
    } else if(S_ISDIR(buf.st_mode)){
        printf("katalog\n");
    } 
    char* path = realpath(".", NULL);
    printf("Sciezka : %s/%s\n", path,name);
    if (S_ISLNK(buf.st_mode))
    {
        free(path);
        char* path = realpath(name, NULL);
        printf("Wskazuje na : %s\n", path);
    }
    free(path);
    printf("Rozmiar : %ld bajtow\n", buf.st_size);
    printf("Uprawnienia: %c", '\0'); 
    //Get Print File Owner Rights
    if(buf.st_mode&S_IRUSR)
        printf("r");
    else
        printf("-");
    if(buf.st_mode&S_IWUSR)
        printf("w");
    else
        printf("-");
    if(buf.st_mode&S_IXUSR)
        printf("x");
    else
        printf("-");

    //All group permissions
    if(buf.st_mode&S_IRGRP)
        printf("r");
    else
        printf("-");
    if(buf.st_mode&S_IWGRP)
        printf("w");
    else
        printf("-");
    if(buf.st_mode&S_IXGRP)
        printf("x");
    else
        printf("-");

    //Other people's rights
    if(buf.st_mode&S_IROTH)
        printf("r");
    else
        printf("-");
    if(buf.st_mode&S_IWOTH)
        printf("w");
    else
        printf("-");
    if(buf.st_mode&S_IXOTH)
        printf("x");
    else
        printf("-");
    printf("\n");
    printf("Ostatnio uzywany:        %s%c", formatdate(buff_time, buf.st_atime), '\n');
    printf("Ostatnio modyfikowany:   %s%c", formatdate(buff_time, buf.st_mtime), '\n');
    printf("Ostatnio zmieniany stan: %s%c", formatdate(buff_time, buf.st_ctime), '\n');
    if (!(buf.st_mode&S_IXOTH && buf.st_mode&S_IXGRP && buf.st_mode&S_IXUSR)){
        printf("Poczatek zawartosci :\n");
        int znak, counter = 0;
        FILE *plik;
        plik = fopen(name, "rt");
        znak = getc(plik);
        while(true){
            if (znak != '\n'){
                printf("%c", znak);
                znak = getc(plik);
            }
            if (znak == '\n'){
                printf("%c", znak);
                counter= counter + 1;
                znak = getc(plik);                
            }
            if (counter == 2){
                break;
            }
        }
        fclose(plik);
    }
}
int main(int argc,char** argv)
{
    int i,num=0;
    char *path[1];
    path[0]=(char*)malloc(sizeof(char)*(PATH_MAX+1));

    if(num+1==argc) {
        strcpy(path[0],".");
        display_dir(path[0]);
        return 0;
    } else if(num+2 == argc) {
        i=1;
        strcpy(path[0],argv[i]);
        display_attribute_detailed(path[0]);
    } else {
        printf("Too Much arguments!\n\nUsage:\n./myls - for Standard ls\n./myls [filename] - for detailed info about file\n\n");
    } 

    return 0;

}