#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<malloc.h>
#include<time.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<dirent.h>
#include<unistd.h>
#include<fcntl.h>
#include<getopt.h>
#include<regex.h>

#define MaxSize 256

typedef struct PaperItem *Paper;
struct PaperItem {
    int id;
    char name[MaxSize];
    int importance;
    int urgency;
    int isRead;
    char time[MaxSize];
    char tag1[MaxSize];
    char tag2[MaxSize];
    char tag3[MaxSize];
    int flag;
    Paper Next;
};

typedef struct FileItem *File;
struct FileItem {
    int id;
    char name[MaxSize];
    char time[MaxSize];
    int flag;
    File Next;
};

typedef struct TagItem *Tag;
struct TagItem {
    char tag[MaxSize];
    Tag Next;
};

char *opt_arg;
const char *short_opts = "rnats:ude:o:hi";
const struct option long_opts[] = {
    {"urg", no_argument, NULL, 'r'},
    {"imp", no_argument, NULL, 'n'},
    {"all", no_argument, NULL, 'a'},
    {"tags", no_argument, NULL, 't'},
    {"sbt", required_argument, NULL, 's'},
    {"sur", no_argument, NULL, 'u'},
    {"sir", no_argument, NULL, 'd'},
    {"edit", required_argument, NULL, 'e'},
    {"open", required_argument, NULL, 'o'},
    {"help", no_argument, NULL, 'h'},
    {"init", no_argument, NULL, 'i'},
    {0, 0, 0, 0}
};
 

void ScanFile(char *dirpath);
void InitPaper(void);
File GetFileName(void);
Paper GetPaper(void);
void PrintAll(Paper paper);
void PrintIsRead(Paper paper);
void PrintUnRead(Paper paper);
void PrintByTag(Paper paper, char tag[]);
Paper OrderByUrg(Paper paper);
Paper OrderByImp(Paper paper);
void InitAll(void);
void screen(void);
void help(void);
void Openpdf(char *pdfName);
void PrintTags(Paper paper);
void Repair(Paper paper, int id);


int main(int argc, char* argv[]) {

    Paper paper, order;
    int c, len, id;
    if ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
        switch (c) {
            case 'r':
                paper = GetPaper();
                order = OrderByUrg(paper);
                PrintAll(order);
                break;
            case 'n':
                paper = GetPaper();
                order = OrderByImp(paper);
                PrintAll(order);
                break;
            case 'a':
                paper = GetPaper();
                PrintAll(paper);
                break;
            case 't':
                paper = GetPaper();
                PrintTags(paper);
                break;
            case 's':
                paper = GetPaper();
                opt_arg = optarg;
                PrintByTag(paper, optarg);
                break;
            case 'u':
                paper = GetPaper();
                PrintUnRead(paper);
                break;
            case 'd':
                paper = GetPaper();
                PrintIsRead(paper);
                break;
            case 'e':
                paper = GetPaper();
                order = paper;
                opt_arg = optarg;
                id = atoi(opt_arg);
                len = 0;
                while(paper != NULL) {
                    paper = paper->Next;
                    len++;
                }
                paper = order;
                if (id > 0 && id <= len) {
                    Repair(paper, atoi(opt_arg));
                } else {
                    printf("The id you entered does not exist.\n");
                    printf("please enter id again.\n");
                }
                break;
            case 'o':
                opt_arg = optarg;
                Openpdf(opt_arg);
                break;
            case 'h':
                help();
                break;
            case 'i':
                InitAll();
                break;
            case '?':
                help();
                break;
            default :
                abort();
        }
    }
    else {
        screen();
        InitAll();
        help();
    }
}

// 初始化
void InitAll(void) {
    File file;
    Paper paper;
    char month[MaxSize], day[MaxSize], hour[MaxSize], year[MaxSize];

    ScanFile(".");
    InitPaper();

    printf("The data has been updated!\n");
}

// 欢迎界面
void screen(void) {
    printf("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");
    printf("%%%%%%%%       %%%%%%%%  %%%%%%%%%%%%%%%%%%%%%%%%   %%%%%%%%\n");
    printf("%%%%%%%%   %%%%%%%%  %%%%   %%%%%%%%%%%%%%%%%%%%    %%%%%%%%\n");
    printf("%%%%%%%%   %%%%%%%%  %%%%    %%%%%%%%%%%%%%%%     %%%%%%%%\n");
    printf("%%%%%%%%   %%%%%%%%  %%%% %%%%  %%%%%%%%%%%%  %%%%  %%%%%%%%\n");
    printf("%%%%%%%%       %%%%%%%% %%%%   %%%%%%%%   %%%%  %%%%%%%%\n");
    printf("%%%%%%%%   %%%%%%%%%%%%%%%% %%%%%%%%  %%%%  %%%%%%%%  %%%%%%%%\n");
    printf("%%%%%%%%   %%%%%%%%%%%%%%%% %%%%%%%%      %%%%%%%%  %%%%%%%%\n");
    printf("%%%%%%%%   %%%%%%%%%%%%%%%% %%%%%%%%%%%%  %%%%%%%%%%%%  %%%%%%%%\n");
    printf("%%%%%%%%   %%%%%%%%%%%%%%%% %%%%%%%%%%%%  %%%%%%%%%%%%  %%%%%%%%\n");
    printf("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");
}

// 帮助界面
void help(void) {
    printf("\nNAME\n");
    printf("\tPaper-Manager 1.0 (2016 Dec. 21)\n\n");

    printf("SYNOPSIS\n");
    printf("\t./a.out  :Auto initialization\n");
    printf("\t./a.out [MODE] [OPTION]\n\n");

    printf("MODE\n");
    printf("\t-r, --urg\t\trecommend the papers according to urgency\n");
    printf("\t-n, --imp\t\trecommend the papers according to importance\n");
    printf("\t-a, --all\t\tshow all papers info\n");
    printf("\t-t, --tags\t\tshow all tags\n");
    printf("\t-s, --sbt\t\tsearch by tag\n");
    printf("\t-u, --sur\t\tsearch articles that not been read\n");
    printf("\t-d, --sir\t\tsearch artivles that have been read\n");
    printf("\t-e, --edit\t\tedit one paper info by paper id\n");
    printf("\t-o, --open\t\topen paper to read by id\n");
    printf("\t-h, --help\t\thelp info\n");
    printf("\t-i, --init\t\tall initialization\n\n");
}

// 打开程序时扫描文件并重建file_name_db
void ScanFile(char *dirpath) {
    DIR *dirp;
    FILE *fp;
   // char fname[MaxSize][MaxSize];

    fp = fopen("file_name_db", "w+");

    struct dirent *dp;
    
    dirp = opendir(dirpath);
    if (dirp == NULL) {
        printf("opendir failed on '%s'", dirpath);
        exit(1);
    }

    int id = 0;
    int flag = 0;                   // 初始化flag为0
   
    regex_t reg;
    int status;
    regmatch_t pmatch[MaxSize];
    const size_t nmatch = MaxSize;
    const char *pattern = ".+\\.pdf$";
    regcomp(&reg, pattern, REG_EXTENDED);

    for (;;) {
        char tmpname[MaxSize];

        dp = readdir(dirp);
        if (dp == NULL) {
            break;
        }

        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }

        strcpy(tmpname, dp->d_name);
        struct stat fname;
        stat(tmpname,&fname);
        status = regexec(&reg, tmpname, nmatch, pmatch, 0);
        if(strstr(tmpname, "pdf")) {
            fprintf(fp, "%d\t%s\t%d\t%s", id+1, dp->d_name, flag, ctime(&fname.st_ctime));
            id++;
        }
    }

    fclose(fp);
    closedir(dirp);
}

// 将file_name_db的信息存放在链表中
File GetFileName(void) {
    File f1, f2, fhead;
    FILE *ffp;

    char month[256], day[256], hour[256], year[256];

    ffp = fopen("file_name_db", "r");
    if (ffp == NULL) {
        printf("the file 'file_name_db' does not exist...\n");
        ScanFile(".");
    }
    else {
        f1 = (File)malloc(sizeof(struct FileItem));
        fhead = f1;
        while(!feof(ffp)) {
            fscanf(ffp, "%d\t%s\t%d\t%s %s %s %s %s\n", &f1->id, &f1->name, &f1->flag,  &f1->time, month,day,hour,year);
            f2 = f1;
            f1 = (File)malloc(sizeof(struct FileItem));
            f2->Next = f1;
        }

        f2->Next = NULL;
        f2 = NULL;
        free(f1);
        f1 = NULL;
        f1 = fhead;
    }

    fclose(ffp);
    return f1;
}

// 第一次使用时根据file_name_db的信息建立paper_info_db，并存放在链表中
// 第二次及以后，直接从paper_info_db中读取信息并建立链表
void InitPaper(void) {
    Paper p1, p2, phead;
    FILE *pfp;

    pfp = fopen("paper_info_db", "r+");
    if (pfp == NULL) {
        pfp = fopen("paper_info_db", "w+");

        printf("\nthe file 'paper_info_db' does not exist!\n");
        printf("Now, you need to initialize it!\n");

        p1 = (Paper)malloc(sizeof(struct PaperItem));
        phead = p1;

        File pf1, pfhead;
        pf1 = GetFileName();
        pfhead = pf1;

        while(pf1 != NULL) {
            int importance, iflag = 0;
            int urgency, uflag = 0;
            int isRead, rflag = 0;
            
            p1->id = pf1->id;
            strcpy(p1->name, pf1->name);
            strcpy(p1->time, pf1->time);
            p1->flag = 0;                           // 初始化flag为0
            printf("\nFind a new paper: %s\n", p1->name);

            while (iflag == 0) {
                printf("Please enter the importance (1~5): ");
                scanf("%d", &importance);
                if (importance >= 1 && importance <= 5) {
                    p1->importance = importance;
                    iflag = 1;
                } else {
                    printf("Please enter again.\n");
                }
            }

            while (uflag == 0) {
                printf("Please enter the urgency (1~5): ");
                scanf("%d", &urgency);
                if (urgency >= 1 && urgency <= 5) {
                    p1->urgency = urgency;
                    uflag = 1;
                } else {
                    printf("Please enter again.\n");
                }
            }

            printf("Please enter tag1: ");
            scanf("%s", &p1->tag1);
            printf("Please enter tag2: ");
            scanf("%s", &p1->tag2);
            printf("Please enter tag3: ");
            scanf("%s", &p1->tag3);
            
            while (rflag == 0) {
                printf("Has the paper been read(1/0): ");
                scanf("%d", &isRead);
                if (isRead == 0 || isRead == 1) {
                    p1->isRead = isRead;
                    rflag = 1;
                } else {
                    printf("Please enter again.\n");
                }
            }

            fprintf(pfp, "%d\t%s\t%d\t%d\t%d\t%s\t%s\t%s\t%s\t%d\n", p1->id,  p1->name, p1->importance, p1->urgency, p1->isRead, p1->time, p1->tag1, p1->tag2, p1->tag3, p1->flag);

            p2 = p1;
            p1 = (Paper)malloc(sizeof(struct PaperItem));
            p2->Next = p1;

            pf1 = pf1->Next;
        }
        
        free(pf1);
        pf1 = pfhead;

        p2->Next = NULL;
        p2 = NULL;
        free(p1);
        p1 = NULL;
        p1 = phead;

        fclose(pfp);
    }
    else {                                           // 取出存放到链表
        p1 = (Paper)malloc(sizeof(struct PaperItem));
        phead = p1;
        while(!feof(pfp)) {
            fscanf(pfp, "%d\t%s\t%d\t%d\t%d\t%s\t%s\t%s\t%s\t%d\n", &p1->id,  &p1->name, &p1->importance, &p1->urgency, &p1->isRead, &p1->time, &p1->tag1, &p1->tag2, &p1->tag3, &p1->flag);
            p2 = p1;
            p1 = (Paper)malloc(sizeof(struct PaperItem));
            p2->Next = p1;
        }

        p2->Next = NULL;
        p2 = NULL;
        free(p1);
        p1 = NULL;
        p1 = phead;
        p2 = phead;

       // fclose(pfp);
       // return p1;
        
        File pf1, pfhead;
        pf1 = GetFileName();
        pfhead = pf1;
        
        // 分别求出两个链表的长度
        int lpf1 = 0;
        while(pf1 != NULL) {
            lpf1++;
            pf1 = pf1->Next;
        }
        pf1 = pfhead;

        int lp1 = 0;
        while(p1 != NULL) {
            lp1++;
            p1 = p1->Next;
        }
        p1 = phead;

        // 将相同节点的flag置为1
        for (int i = 0; i < lpf1; i++) {
            for (int j = 0; j < lp1; j++) {
                if (strcmp(p1->name, pf1->name) == 0) {
                    p1->flag = 1;
                    pf1->flag = 1;
                }
                p1 = p1->Next;
            }
            p1 = phead;
            pf1 = pf1->Next;
        }
        pf1 = pfhead;
        p1 = phead;

        // 遍历file_name_db的链表，找出flag为0的节点，即新添加的文件
        int addNum = 0;
        while(pf1 != NULL) {
            int importance, iflag = 0;
            int urgency, uflag = 0;
            int isRead, rflag = 0;
            
            if (pf1->flag == 0) {
                if (addNum == 0) {
                    printf("The following papers are new:\n");
                    addNum = 1;
                }

                while(p2->Next !=NULL) {
                    p2 = p2->Next;
                }

                printf("\nFind a new paper: %s\n\n", pf1->name);

                p1 = (Paper)malloc(sizeof(struct PaperItem));
                p2->Next = p1;
                
                
                p1->id = pf1->id;
                strcpy(p1->name, pf1->name);
                strcpy(p1->time, pf1->time);
                p1->flag = 1;

                while (iflag == 0) {
                    printf("Please enter the importance (1~5): ");
                    scanf("%d", &importance);
                    if (importance >= 1 && importance <= 5) {
                        p1->importance = importance;
                        iflag = 1;
                    } else {
                        printf("Please enter again.\n");
                    }
                }

                while (uflag == 0) {
                    printf("Please enter the urgency (1~5): ");
                    scanf("%d", &urgency);
                    if (urgency >= 1 && urgency <= 5) {
                        p1->urgency = urgency;
                        uflag = 1;
                    } else {
                        printf("Please enter again.\n");
                    }
                }

                printf("Please enter tag1: ");
                scanf("%s", &p1->tag1);
                printf("Please enter tag2: ");
                scanf("%s", &p1->tag2);
                printf("Please enter tag3: ");
                scanf("%s", &p1->tag3);

                while (rflag == 0) {
                printf("Has the paper been read(1/0): ");
                scanf("%d", &isRead);
                if (isRead == 0 || isRead == 1) {
                    p1->isRead = isRead;
                    rflag = 1;
                } else {
                    printf("Please enter again.\n");
                }
            }

                p2 = p2->Next;
                p2->Next = NULL;
            }
            pf1 = pf1->Next;
        }
        pf1 = pfhead;
        p1 = phead;
        p2 = phead;

        // 遍历paper_info_db的链表，找出flag为0的节点，即已删除的文件
        int delNum = 0;
        while(p1 != NULL) {
            if (p1->flag != 0) {
                p2 = p1;
                p1 = p1->Next;
            }
            else {
                if(delNum == 0) {
                    printf("\nThe following papers have been deleted:\n");
                    delNum = 1;
                }
                printf("\t%s\n", p1->name);
                
                p2->Next = p1->Next;
                p1->Next = NULL;
                free(p1);
                p1 = p2->Next;
            }
        }
        p1 = phead;
        p2 = phead;

        // 将更新写入paper_info_db
        int nid = 0;                        // 重新分配id
        pfp = fopen("paper_info_db", "w+");
        while(p1 != NULL) {
            p1->flag = 0;                   // 重置flag
            nid++;
            fprintf(pfp, "%d\t%s\t%d\t%d\t%d\t%s\t%s\t%s\t%s\t%d\n", nid,  p1->name, p1->importance, p1->urgency, p1->isRead, p1->time, p1->tag1, p1->tag2, p1->tag3, p1->flag);

            p1 = p1->Next;
        }
        p1 = phead;

        fclose(pfp);
    }
}

// 调用默认程序打开pdf
void Openpdf(char *pdfName) {
    char *program = "evince ";
    char *cmd = malloc(strlen(program) + strlen(pdfName) + 1);
    if (cmd == NULL) {
        exit(1);
    }

    strcpy(cmd, program);
    strcat(cmd, pdfName);

    system(cmd);
}

// 从paper_info_db获得链表
Paper GetPaper(void) {
    Paper p1, p2, phead;
    FILE *pfp;

    p1 = (Paper)malloc(sizeof(struct PaperItem));
    pfp = fopen("paper_info_db", "r");
    if (pfp == NULL) {
        printf("You have not initialized yet.\n");
        help();
        fclose(pfp);
        exit(1);
    }
    else {
        phead = p1;
        while (!feof(pfp)) {
            fscanf(pfp, "%d\t%s\t%d\t%d\t%d\t%s\t%s\t%s\t%s\t%d\n", &p1->id,  &p1->name, &p1->importance, &p1->urgency, &p1->isRead, &p1->time, &p1->tag1, &p1->tag2, &p1->tag3, &p1->flag);
            p2 = p1;
            p1 = (Paper)malloc(sizeof(struct PaperItem));
            p2->Next = p1;
        }

        p2->Next = NULL;
        p2 = NULL;
        free(p1);
        p1 = NULL;
        p1 = phead;
        p2 = phead;

        fclose(pfp);
        return p1;
    }
}

// 全部打印
void PrintAll(Paper paper) {
    Paper p1 = paper;
    printf("id\tname\timp\turg\tRead\ttime\ttags\n");
    while (p1 != NULL) {
        printf("%d\t%s\t%d\t%d\t%d\t%s\t%s,%s,%s\n", p1->id,  p1->name, p1->importance, p1->urgency, p1->isRead, p1->time, p1->tag1, p1->tag2, p1->tag3);
        p1 = p1->Next;
    }
}

// 打印已读的文章
void PrintIsRead(Paper paper) {
    Paper p1 = paper;
    printf("id\tname\timp\turg\tRead\ttime\ttags\n");
    while (p1 != NULL) {
        if (p1->isRead == 1) {
            printf("%d\t%s\t%d\t%d\t%d\t%s\t%s,%s,%s\n", p1->id,  p1->name, p1->importance, p1->urgency, p1->isRead, p1->time, p1->tag1, p1->tag2, p1->tag3);
        }
        p1 = p1->Next;
    }
}

// 打印未读的文章
void PrintUnRead(Paper paper) {
    Paper p1 = paper;
    printf("id\tname\timp\turg\tRead\ttime\ttags\n");
    while (p1 != NULL) {
        if (p1->isRead == 0) {
            printf("%d\t%s\t%d\t%d\t%d\t%s\t%s,%s,%s\n", p1->id,  p1->name, p1->importance, p1->urgency, p1->isRead, p1->time, p1->tag1, p1->tag2, p1->tag3);
        }
        p1 = p1->Next;
    }
}

// 按紧迫程度排序,重要性相同时再按紧迫性排序
Paper OrderByUrg(Paper paper) {
    Paper p1, p2;
    Paper p;
    Paper endpt;

    p1 = (Paper)malloc(sizeof(struct PaperItem));
    p1->Next = paper;
    paper = p1;

    for (endpt =NULL; endpt != paper; endpt = p) {
        for (p = p1 = paper; p1->Next->Next != endpt; p1 = p1->Next) {
            if (p1->Next->urgency < p1->Next->Next->urgency || (p1->Next->urgency == p1->Next->Next->urgency && p1->Next->importance < p1->Next->Next->importance)) {
                p2 = p1->Next->Next;
                p1->Next->Next = p2->Next;
                p2->Next = p1->Next;
                p1->Next = p2;
                p = p1->Next->Next;
            }
        }
    }

    p1 = paper;
    paper = paper->Next;
    free(p1);
    p1 = paper;

    return paper;
}

// 按重要性排序，重要性相同时再按重要性排序
Paper OrderByImp(Paper paper) {
    Paper p1, p2;
    Paper p;
    Paper endpt;

    p1 = (Paper)malloc(sizeof(struct PaperItem));
    p1->Next = paper;
    paper = p1;

    for (endpt =NULL; endpt != paper; endpt = p) {
        for (p = p1 = paper; p1->Next->Next != endpt; p1 = p1->Next) {
            if (p1->Next->importance < p1->Next->Next->importance || (p1->Next->importance == p1->Next->Next->importance && p1->Next->urgency < p1->Next->Next->urgency)) {
                p2 = p1->Next->Next;
                p1->Next->Next = p2->Next;
                p2->Next = p1->Next;
                p1->Next = p2;
                p = p1->Next->Next;
            }
        }
    }

    p1 = paper;
    paper = paper->Next;
    free(p1);
    p1 = paper;

    return paper;
 }

// 输入tag，查找相应的结点并打印
void PrintByTag(Paper paper, char tag[]) {
    Paper p1 = paper;
    printf("id\tname\timp\turg\tRead\ttime\ttags\n");
    while (p1 != NULL) {
        if ((strcmp(p1->tag1, tag) && strcmp(p1->tag2, tag) && strcmp(p1->tag3, tag)) == 0) {
            printf("%d\t%s\t%d\t%d\t%d\t%s\t%s,%s,%s\n", p1->id,  p1->name, p1->importance, p1->urgency, p1->isRead, p1->time, p1->tag1, p1->tag2, p1->tag3);
        }
        p1 = p1->Next;
    }
}

// 读取tag建立链表，去重后打印
void PrintTags(Paper paper) {
    Paper p1, phead;
    p1 = paper;
    phead = p1;

    Tag t1, t2, thead;
    t1 = (Tag)malloc(sizeof(struct TagItem));
    thead = t1;

    // 建立链表
    while (p1 != NULL) {
        strcpy(t1->tag, p1->tag1);
        t2 = t1;
        t1 = (Tag)malloc(sizeof(struct TagItem));
        t2->Next = t1;

        strcpy(t1->tag, p1->tag2);
        t2 = t1;
        t1 = (Tag)malloc(sizeof(struct TagItem));
        t2->Next = t1;

        strcpy(t1->tag, p1->tag3);
        t2 = t1;
        t1 = (Tag)malloc(sizeof(struct TagItem));
        t2->Next = t1;

        p1 = p1->Next;
    }

    p1 = phead;
    t2->Next = NULL;
    t2 = thead;
    free(t1);
    t1 = thead;

    // 去重
    t1 = t1->Next;
    Tag tmp1 = t1, tmp2 = t2;
    while (t2 != NULL) {
        t1 = t2->Next;
        tmp1 = t1;

        while (t1 != NULL) {
            if (strcmp(t2->tag, t1->tag) == 0) {
                t1 = t1->Next;
                tmp2->Next = t1;
                tmp1->Next = NULL;
                free(tmp1);
                tmp1 = t1;
            } else {
                tmp2 = t1;
                t1 = t1->Next;
                tmp1 = t1;
            }
        }
        t2 = t2->Next;
        tmp2 = t2;
    }
    t2 = thead;
    t1 = thead;

    // 打印
    while (t1 != NULL) {
        printf("%s  ", t1->tag);
        t1 = t1->Next;
    }
    printf("\n");
}

// 修改信息并存入paper_info_db
void Repair(Paper paper, int id) {
    Paper p1 = paper;
    FILE *rp;
    int importance, urgency, isRead;
    int num, con = 0, iflag = 0, uflag = 0, rflag = 0;

    while (p1 != NULL) {
        if (p1->id == id) {
            break;
        } else {
            p1 = p1->Next;
        }
    }
    printf("The orginal information you want to modify is:\n");
    printf("id\tname\timp\turg\tRead\ttime\ttags\n");
    printf("%d\t%s\t%d\t%d\t%d\t%s\t%s,%s,%s\n", p1->id,  p1->name, p1->importance, p1->urgency, p1->isRead, p1->time, p1->tag1, p1->tag2, p1->tag3);
   
    do {
        printf("Please enter the name of the information you want to edit.\n");
        printf("1.importance 2.urgency 3.isRead 4.tag1 5.tag2 6.tag3\n");
        printf("Your choice is: ");
        scanf("%d", &num);
        switch(num) {
            case 1: while (iflag == 0) {
                        printf("Please enter new importance [1~5]: ");
                        scanf("%d", &importance);
                        if (importance >= 1 && importance <= 5) {
                            p1->importance = importance;
                        } else {
                            printf("Please enter again.\n");
                        }
                    }
                    break;
            case 2: while (uflag == 0) {
                        printf("Please enter new urgency [1~5]: ");
                        scanf("%d", &urgency);
                        if (urgency >= 1 && urgency <= 5) {
                            p1->urgency = urgency;
                            uflag = 1;
                        } else {
                            printf("Please enter again.\n");
                        }
                    }
                    break;
            case 3: while (rflag == 0) {
                        printf("Has the paper been read [1/0]: ");
                        scanf("%d", &isRead);
                        if (isRead == 0 || isRead == 1) {
                            p1->isRead = isRead;
                        } else {
                            printf("Please enter again.\n");
                        }
                    }
                    break;
            case 4: printf("Please enter new tag1: ");
                    scanf("%s", p1->tag1);
                    break;
            case 5: printf("Please enter new tag2: ");
                    scanf("%s", p1->tag2);
                    break;
            case 6: printf("Please enter new tag3: ");
                    scanf("%s", p1->tag3);
                    break;
            default:printf("Please enter again.\n");
                    break;
        }
        printf("Whether to continue to modify this item [1/0]: ");
        scanf("%d", &con);
        if (con == 1) {
            continue;
        } else {
            break;
        }
    } while (con == 1);

    printf("The changed information is:\n");
    printf("id\tname\timp\turg\tRead\ttime\ttags\n");
    printf("%d\t%s\t%d\t%d\t%d\t%s\t%s,%s,%s\n", p1->id,  p1->name, p1->importance, p1->urgency, p1->isRead, p1->time, p1->tag1, p1->tag2, p1->tag3);

    p1 = paper;
    rp = fopen("paper_info_db", "w+");
    while(p1 != NULL) {
        fprintf(rp, "%d\t%s\t%d\t%d\t%d\t%s\t%s\t%s\t%s\t%d\n", p1->id,  p1->name, p1->importance, p1->urgency, p1->isRead, p1->time, p1->tag1, p1->tag2, p1->tag3, p1->flag);
        p1 = p1->Next;
    }
    p1 = paper;
    fclose(rp);
}

