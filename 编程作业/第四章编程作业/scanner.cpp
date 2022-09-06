#include<stdio.h>
#include <iostream>
#include<stdlib.h>
#include<map>
#include<vector>
#include<string>
#include<string.h>

#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>  

using namespace std;

map<int, vector<string>> record ;

char root[6];

void ScannerFile(char* dirPath)
{
    DIR *dir=opendir(dirPath);
    if (dir == NULL)
    {
        printf("Failed to open the path:%s\n",dirPath);
    }
    //加入对应inode,filename的记录
    else
    {

        chdir(dirPath);//将工作路径切换到当前目录

        struct dirent* Direct;//创建目录项指针
        while ((Direct = readdir(dir))!=NULL)
	    {
            record[Direct->d_ino].push_back(string(Direct->d_name));
		    if(strcmp(Direct->d_name,".")== 0||strcmp(Direct->d_name,"..")== 0)
		    {
			    continue;
		    }
            
		    struct stat inode;
		    stat(Direct->d_name,&inode);//读出对应的索引节点的内容来判断文件类型
		    if(S_ISDIR(inode.st_mode)) //判断如果为目录文件，则递归调用函数访问子目录文件
		    {
			    ScannerFile(Direct->d_name);
		    }
	    }
        closedir(dir);//关闭当前的目录文件
        if (strcmp(dirPath,root) != 0)
	        chdir("..");//返回上级目录，也就是返回调用该函数之前的工作目录
    }
    return;
}

int main()
{
    root[0] = '/';
    root[1] = 'h';
    root[2] = 'o';
    root[3] = 'm';
    root[4] = 'e';
    root[5] = '\0';
    /*root[0] = '/';
    root[1] = '\0';*/

    chdir(root);//改变当前工作路径为根目录
    //system("ls");

    ScannerFile(root);//从当前文件系统根目录开始扫描文件
    int count = 0;
    for (map<int, vector<string>>::iterator iter = record.begin(); iter != record.end(); ++ iter)
    {
        
        if ((iter->second).size() > 1)
        {
            ++count;
            cout << "inode: " << iter->first << " ";
            for(vector<string>::iterator i = (iter->second).begin(); i != (iter->second).end(); ++ i)
            {
                cout << (*i) << "  ";
            }
            cout << endl;
        }
    }
    cout << "Count:"<<count<<endl;
    return 0;
}