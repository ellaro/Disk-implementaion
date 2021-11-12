#include <iostream>
#include <vector>
#include <map>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <cmath>

using namespace std;

#define DISK_SIZE 256


char  decToBinary(int n , char &c)
{
    // array to store binary number
    int binaryNum[8];

    // counter for binary array
    int i = 0;
    while (n > 0) {
        // storing remainder in binary array
        binaryNum[i] = n % 2;
        n = n / 2;
        i++;
    }

    // printing binary array in reverse order
    for (int j = i - 1; j >= 0; j--) {
        if (binaryNum[j]==1)
            c = c | 1u << j;
    }
}


// ============================================================================

class FsFile {

    int file_size;

    int block_in_use;

    int index_block;

    int block_size;





    public:

        FsFile(int _block_size) {

            file_size = 0;

            block_in_use = 0;

            block_size = _block_size;

            index_block = -5;

        }



        int getfile_size(){

            return file_size;

        }
        void setfile_size(int len){
            file_size=len;

        }
        int getBlock_in_use(){
            return block_in_use;
        }
        void setBlock_in_use(int block){
            block_in_use=block;
        }

        int getIndex_block(){
            return index_block;
        }
        void setIndex_block(int ind){
            index_block=ind;
        }
        int getblock_size(){
            return block_size;
        }
        void setblock_size(int size){
            block_size=size;
        }



};


// ============================================================================

class FileDescriptor {

    string file_name;

    FsFile* fs_file;

    bool inUse; //open = true, closed = false



    public:

        FileDescriptor(string FileName, FsFile* fsi) {
            

            file_name = FileName;

            fs_file = fsi;

            inUse = true;

        }

        string getFileName() {

            return file_name;

        }
        FsFile* getfile(){
            return fs_file;
        }

        bool getInUse() {
            return (inUse);
        }
        void setInUse(bool use) {
            inUse = use ;
        }
        void deleteName(){
        file_name = "";
        }



    };

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"

// ============================================================================

class fsDisk {
    


    FILE *sim_disk_fd;

    bool is_formated;



    // BitVector - "bit" (int) vector, indicate which block in the disk is free

    //              or not.  (i.e. if BitVector[0] == 1 , means that the

    //             first block is occupied.

    int BitVectorSize;

    int *BitVector;



    // filename and one fsFile.



    map<string, FsFile *> MainDir;



    // OpenFileDescriptors --  when you open a file,

    // the operating system creates an entry to represent that file

    // This entry number is the file descriptor.



    vector<FileDescriptor> OpenFileDescriptors;



    int block_size;

    int maxSize;

    int freeBlocks;

    private:
            bool withSpaces(int required_blocks){
                int count = 0;
                for(int i = 0; i < BitVectorSize; i++)
                    if(BitVector[i] == 0)//Free
                        count++;

                if(count >= required_blocks)
                    return true;
                return false;
            }
            int getFreeBlock(){
                for(int i = 0; i < BitVectorSize; i++)
                    if(BitVector[i] == 0){
                        BitVector[i] = 1;//Taken
                        return i;
                    }

                return -1;
            }



    // ------------------------------------------------------------------------
    public:

        fsDisk() {
            

            sim_disk_fd = fopen(DISK_SIM_FILE , "r+");

            assert(sim_disk_fd);

            for (int i=0; i < DISK_SIZE ; i++) {

                int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );

                ret_val = fwrite( "\0" ,  1 , 1, sim_disk_fd);

                assert(ret_val == 1);

            }

            fflush(sim_disk_fd);

            block_size = 0;

            is_formated = false;

        }

    
        // ------------------------------------------------------------------------
            void listAll() { //prints all the files

                int i = 0;

                for (auto foo = begin(OpenFileDescriptors); foo != end (OpenFileDescriptors); ++foo) {

                    cout << "index: " << i << ": FileName: " << foo->getFileName()  <<  " , isInUse: " << foo->getInUse()<< endl;

                    i++;

                }

                char bufy;

                cout << "Disk content: '" ;

                for (i=0; i < DISK_SIZE ; i++) {

                    int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );

                    ret_val = fread(  &bufy , 1 , 1, sim_disk_fd );
                    if((int)bufy>32){
                        cout << bufy;
                    }
                }


                cout << "'" << endl;

            }



            // ------------------------------------------------------------------------
            void fsFormat( int blockSize =4 ) {
                if(is_formated) {
                    free(BitVector);
                }
                this->block_size = blockSize;
                this->BitVectorSize = DISK_SIZE/block_size; //256/4=64
                BitVector=new int[BitVectorSize]; //indicate which block in the disk is free
                assert(BitVector);
                memset(BitVector, 0, BitVectorSize);
                this->is_formated = true;

            }

            // ------------------------------------------------------------------------
            int CreateFile(string fileName) {
                if(!is_formated) {
                    return -1;
                }
                if(MainDir.find(fileName) != MainDir.end()){
                    return -1;
                }
                FsFile *fs = new FsFile(this->block_size);
                MainDir.insert({fileName, fs});
                OpenFileDescriptors.push_back(FileDescriptor(fileName, fs));
                return (OpenFileDescriptors.size() - 1);
            }

            // ------------------------------------------------------------------------
            int OpenFile(string fileName) {
                if(!is_formated || MainDir.find(fileName) == MainDir.end()) {
                    return -1;
                }
                for(int i = 0; i < OpenFileDescriptors.size(); i++)
                    if(OpenFileDescriptors.at(i).getFileName() == fileName){
                        bool inUse = OpenFileDescriptors.at(i).getInUse();
                        if(inUse){
                            return -1;//Already opens
                        }else if(!inUse){
                            OpenFileDescriptors.at(i).setInUse(true);

                            return i;
                        }
                    }
            }

            // ------------------------------------------------------------------------
            string CloseFile(int fd) {
                if( !OpenFileDescriptors.at(fd).getInUse()|| !is_formated || fd > OpenFileDescriptors.size() - 1) {
                    return "-1";
                }
                string name = OpenFileDescriptors.at(fd).getFileName();
                if(MainDir.find(name) == MainDir.end())
                    return "-1";

                OpenFileDescriptors.at(fd).setInUse(false);
                return name;
            }

            // ------------------------------------------------------------------------
            int WriteToFile(int fd, char *buf, int len ) {
                if(!is_formated || !OpenFileDescriptors.at(fd).getInUse()|| fd > OpenFileDescriptors.size() - 1) {
                    return -1;
                }
                FsFile *temp = OpenFileDescriptors.at(fd).getfile();
                // int curBlocks;
                int curSize = temp->getfile_size();
                int reqBlocks = (int)ceil((len/(double)block_size));
                if((curSize + len > block_size*block_size ) || !(withSpaces(reqBlocks)))//Too big from maximum file or not enough space on disk
                    return -1;

                int disk_fd = fileno(sim_disk_fd);
                char* p = buf;
                int indexBlock = temp->getIndex_block();
                int blockInUse=temp->getBlock_in_use();
                int write_count=block_size;
                int currlen=len;
                if(blockInUse==0){//if virgin file
                    if(len<block_size){
                        write_count=len;
                        reqBlocks=1;
                    }
                  
                    temp->setIndex_block(getFreeBlock());
                    indexBlock=temp->getIndex_block();
                    for(int i=0;i<reqBlocks;i++){
                        int indTemp=getFreeBlock();
                        assert(lseek(disk_fd,indTemp*write_count,SEEK_SET)>=0);
                        assert(write(disk_fd, p, write_count)>=0);
                        char shrink ='\0';
                        decToBinary(indTemp, shrink);
                        char shrink_buff[2];
                        shrink_buff[0] = shrink;
                        shrink_buff[1] = '\0';
                        assert(lseek(disk_fd,indexBlock*write_count + i,SEEK_SET)>=0);
                        assert(write(disk_fd, shrink_buff, 1)>=0);
                        p += block_size;
                    }
    
                    temp->setfile_size(len);
                    temp->setBlock_in_use(reqBlocks);
                }
                else{ 
                    
                    indexBlock=temp->getIndex_block();
                    char tempBuf [block_size];
                    char arr[1];
                    assert(lseek(disk_fd,(indexBlock*block_size)+blockInUse-1,SEEK_SET)>=0);
                    assert(read(disk_fd, arr, 1)>=0);
                    int p_addr= (int)*arr;
                    assert(lseek(disk_fd,(p_addr*block_size),SEEK_SET)>=0);
                    assert(read(disk_fd, tempBuf, block_size)>=0);
                    
                    int offSet=0;
                    for(int i=0;i<block_size;i++){
                        if(!tempBuf[i]){
                            offSet++;
                        }
                    
                    }
                    if((blockInUse-1)*block_size+block_size-offSet+len > block_size*block_size){
                        return -1;
                    }
                    if(len<block_size){
                        write_count=len;
                        reqBlocks=1;

                    }
                    else{
                        reqBlocks=(int)((len-offSet)/block_size);
                    }
                    int lenCopy=len;
                    if(offSet!=0){
                        assert(lseek(disk_fd,(p_addr*block_size)+block_size-offSet,SEEK_SET)>=0);
                        assert(write(disk_fd, p, offSet)>=0);
                        p+=offSet;
                        lenCopy-=offSet;
                    }

                    int cur_Blocks=blockInUse;
                  
                     for(int i=0;i<reqBlocks;i++){
                        if(lenCopy<block_size){
                            write_count=lenCopy;
                        }
                        int indTemp=getFreeBlock();
                        assert(lseek(disk_fd,indTemp*block_size,SEEK_SET)>=0);
                        assert(write(disk_fd, p, write_count)>=0);
                        char shrink ='\0';
                        decToBinary(indTemp, shrink);
                        char shrink_buff[2];
                        shrink_buff[0] = shrink;
                        shrink_buff[1] = '\0';
                        assert(lseek(disk_fd,indexBlock*block_size+(cur_Blocks++) ,SEEK_SET)>=0);
                        assert(write(disk_fd, shrink_buff, 1)>=0);
                        p += block_size;
                        lenCopy-=block_size;
                    }
                    temp->setfile_size(len+temp->getfile_size());
                    temp->setBlock_in_use(blockInUse+reqBlocks);
                

                }
                  
            
        }



            // ------------------------------------------------------------------------
            int DelFile( string FileName ) {
                if(!is_formated ) {
                    return -1;
                }
                if(MainDir.find(FileName) == MainDir.end()){//didnt find
                    return -1;
                }
                FsFile *temp = MainDir.find(FileName)->second;
                int disk_fd = fileno(sim_disk_fd);
                if(temp->getfile_size() == 0)
                    return -1;
                int blockInUse=temp->getBlock_in_use();
                int fd = 0;
                int indexBlock=temp->getIndex_block();
                char writeBlocks[block_size];
                 memset(writeBlocks,'\0',block_size);
                char addr[1];
                string res="";
                int write_count=block_size;
                if(blockInUse<block_size){
                    write_count=blockInUse;
                }
                int b[blockInUse];
                for(int i = 0; i < blockInUse; i++){
                    assert(lseek(disk_fd,(indexBlock*write_count)+i,SEEK_SET)>=0);
                    assert(read(disk_fd,addr,1)>=0);
                    int mamir=(int)addr[0];
                    b[i]=mamir;
                    assert(lseek(disk_fd,(mamir*write_count),SEEK_SET)>=0);
                    assert(write(disk_fd,writeBlocks,write_count)>=0);
        
                }
                for(int i=0;i<blockInUse;i++){
                    BitVector[b[i]]=0;

                }
                temp->setIndex_block(-1);
                BitVector[indexBlock]=0;
                delete(MainDir.at(FileName));
                MainDir.erase(FileName);
                
                
                return 0;
            
            }
            // ------------------------------------------------------------------------
            int ReadFromFile(int fd, char *buf, int len ) {
                if(!is_formated || !OpenFileDescriptors.at(fd).getInUse()|| fd > OpenFileDescriptors.size() - 1) {
                    return -1;
                }
                FsFile *temp = OpenFileDescriptors.at(fd).getfile();
                if(temp->getfile_size() == 0){//Empty file
                    buf[0] = 0;
                    return 0;
                }
                int read_count=block_size;
                if(len<block_size){
                    read_count=len;
                }
                int disk_fd = fileno(sim_disk_fd);
                int indexBlock=temp->getIndex_block();
                char readBlocks[block_size];
                char addr[1];
                string res="";
                for(int i=0;i<temp->getBlock_in_use();i++){
                    assert(lseek(disk_fd,(indexBlock*block_size)+i,SEEK_SET)>=0);
                    assert(read(disk_fd,addr,1)>=0);
                    int mamir=(int)addr[0];
                    assert(lseek(disk_fd,(mamir*block_size),SEEK_SET)>=0);
                    assert(read(disk_fd,readBlocks,block_size)>=0);
                    readBlocks[block_size]=NULL;
                    
                    string cool=(string)readBlocks;
                    cool=cool.substr(0,block_size);
                    res+=cool;
                }
                int j=0;
                for(int i=0;i<len;i++){

                    if(!res.at(i)){
                        continue;
                    }
                    buf[j]=res.at(i);
                    j++;
                }
                buf[j]='\0';
                cout<<j;
                
                return 0;
            }
};



int main() {
    int blockSize;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read;
    int _fd;
    cout << "hello im alive";
    fsDisk *fs = new fsDisk();
    int cmd_;
    while(1) {
        cin >> cmd_;
        switch (cmd_)
        {
            case 0:   // exit
                delete fs;
                exit(0);
                break;

            case 1:  // list-file
                fs->listAll();
                break;

            case 2:    // format
                cin >> blockSize;
                fs->fsFormat(blockSize);
                break;

            case 3:    // creat-file
                cin >> fileName;
                _fd = fs->CreateFile(fileName);
                cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 4:  // open-file
                cin >> fileName;
                _fd = fs->OpenFile(fileName);
                cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 5:  // close-file
                cin >> _fd;
                fileName = fs->CloseFile(_fd);
                cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

           case 6:   // write-file
               cin >> _fd;
               cin >> str_to_write;
               fs->WriteToFile( _fd , str_to_write , strlen(str_to_write) );
               break;

           case 7:    // read-file
               cin >> _fd;
               cin >> size_to_read ;

               memset(str_to_read,NULL,DISK_SIZE);
               fs->ReadFromFile( _fd , str_to_read , size_to_read );
               cout << "ReadFromFile: " << str_to_read << endl;
               break;

           case 8:   // delete file
               cin >> fileName;
               _fd = fs->DelFile(fileName);
               cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
               break;
            default:
                break;
        }
    }

}
