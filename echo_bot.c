#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
 
#include <unistd.h>
 
#include <sodium/utils.h>
#include <tox/tox.h>

// struct should be moved into a header file

// original form https://github.com/GrayHatter/uTox/blob/develop/file_transfers.h
#define MAX_FILE_TRANSFERS 32
#define MAX_NUM_FRIENDS 32  // TODO find the right value. what does this mean?
typedef struct FILE_TRANSFER {
    _Bool    in_use;
    uint32_t friend_number, file_number;
    uint8_t  file_id[TOX_FILE_ID_LENGTH];
    uint8_t  status, resume, kind;
    _Bool    incoming, in_memory, is_avatar; //, in_tmp_loc;
    uint8_t  *path, *name; //, *tmp_path;
    size_t   path_length, name_length; //, tmp_path_length;
    uint64_t size, size_transferred;
    uint8_t  *memory, *avatar;

    /* speed + progress calculations. */
    uint32_t speed, num_packets;
    uint64_t last_check_time, last_check_transferred;

    FILE *file, *saveinfo;
    //MSG_FILE *ui_data;
} FILE_TRANSFER;

// from https://github.com/GrayHatter/uTox/blob/develop/file_transfers.c
//static FILE_TRANSFER *file_t[256], **file_tend = file_t;
static FILE_TRANSFER outgoing_transfer[MAX_NUM_FRIENDS][MAX_FILE_TRANSFERS] = {{{0}}};
static FILE_TRANSFER incoming_transfer[MAX_NUM_FRIENDS][MAX_FILE_TRANSFERS] = {{{0}}};

// Remove if supported by core
#define TOX_FILE_KIND_EXISTING 3

FILE_TRANSFER *get_file_transfer(uint32_t friend_number, uint32_t file_number){
    _Bool incoming = 0;
    if (file_number >= (1 << 16)) {
        file_number = (file_number >> 16) - 1;
        incoming = 1;
    }

    if (file_number >= MAX_FILE_TRANSFERS)
        return NULL;

    FILE_TRANSFER *ft;
    if (incoming) {
        ft = &incoming_transfer[friend_number][file_number];
        ft->incoming = 1;
    } else {
        ft = &outgoing_transfer[friend_number][file_number];
        ft->incoming = 0;
    }

    return ft;
}



/* Create a FILE_TRANSFER struct with the supplied data. */
static void utox_build_file_transfer(FILE_TRANSFER *ft, uint32_t friend_number, uint32_t file_number,
    uint64_t file_size, _Bool incoming, _Bool in_memory, _Bool is_avatar, uint8_t kind, const uint8_t *name,
    size_t name_length, const uint8_t *path, size_t path_length, const uint8_t *file_id, Tox *tox){
    FILE_TRANSFER *file = ft;

    memset(file, 0, sizeof(FILE_TRANSFER));

    file->friend_number = friend_number;
    file->file_number   = file_number;
    file->size          = file_size;

    file->incoming      = incoming;
    file->in_memory     = in_memory;
    file->is_avatar     = is_avatar;
    file->kind          = kind;

    if(name){
        file->name        = malloc(name_length + 1);
        memcpy(file->name, name, name_length);
        file->name_length = name_length;
        file->name[file->name_length] = 0;
    } else {
        file->name        = NULL;
        file->name_length = 0;
    }

    if(path){
        file->path        = malloc(path_length + 1);
        memcpy(file->path, path, path_length);
        file->path_length = path_length;
        file->path[file->path_length] = 0;
    } else {
        file->path        = NULL;
        file->path_length = 0;
    }

    if(file_id){
        memcpy(file->file_id, file_id, TOX_FILE_ID_LENGTH);
    } else {
        tox_file_get_file_id(tox, friend_number, file_number, file->file_id, 0);
    }

    // TODO size correction error checking for this...
    if(in_memory){
        if (is_avatar){
            file->avatar = calloc(file_size, sizeof(uint8_t));
        } else {
            file->memory = calloc(file_size, sizeof(uint8_t));
        }
    }

    if(!incoming){
        /* Outgoing file */
          printf("comented out FILE_TRANSFER_STATUS_PAUSED_THEM");
//        file->status = FILE_TRANSFER_STATUS_PAUSED_THEM;
    }
}

static void utox_new_user_file(FILE_TRANSFER *file){
    FILE_TRANSFER *file_copy = calloc(1, sizeof(FILE_TRANSFER));

    memcpy(file_copy, file, sizeof(FILE_TRANSFER));
    printf("%s\n", file_copy);
    printf("%s\n", file);
    printf("%i\n", sizeof(FILE_TRANSFER));
    postmessage(FRIEND_FILE_NEW, 0, 0, file_copy);
}

/* Function called by core with a new file send request from a friend. */
static void incoming_file_callback_request(Tox *tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size, const uint8_t *filename, size_t filename_length, void *user_data){
    printf("HIER incoming_file_callback_request\n");
    /* First things first, get the file_id from core */
    uint8_t file_id[TOX_FILE_ID_LENGTH] = {0};
    tox_file_get_file_id(tox, friend_number, file_number, file_id, 0);
    /* access the correct memory location for this file */
    FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);
    if(!file_handle) {
//        debug("FileTransfer:\tUnable to get memory handle for transfer, canceling friend/file number (%u/%u)\n", friend_number, file_number);
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, 0);
        return;
    }

    switch(kind){
    case TOX_FILE_KIND_AVATAR:{
    printf("In the avatar case.");
    }
    case TOX_FILE_KIND_DATA:{
        /* Load saved information about this file */
        file_handle->friend_number = friend_number;
        file_handle->file_number   = file_number;
        memcpy(file_handle->file_id, file_id, TOX_FILE_ID_LENGTH);

            utox_build_file_transfer(file_handle, friend_number, file_number, file_size, 1, 0, 0,
                TOX_FILE_KIND_DATA, filename, filename_length, NULL, 0, NULL, tox);
            /* Set UI values */
//            file_handle->ui_data = message_add_type_file(file_handle);
            file_handle->resume = 0;
            /* Notify the user! */
            utox_new_user_file(file_handle);
//        }
        break; /*We shouldn't reach here, but just in case! */
    } /* last case */
    } /* switch */
}




typedef struct DHT_node {
    const char *ip;
    uint16_t port;
    const char key_hex[TOX_PUBLIC_KEY_SIZE*2 + 1];
    unsigned char key_bin[TOX_PUBLIC_KEY_SIZE];
} DHT_node;

 
const char *savedata_filename = "savedata.tox";
const char *savedata_tmp_filename = "savedata.tox.tmp";
 
Tox *create_tox()
{
    Tox *tox;
 
    struct Tox_Options options;
 
    tox_options_default(&options);
 
    FILE *f = fopen(savedata_filename, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
 
        char *savedata = malloc(fsize);
 
        fread(savedata, fsize, 1, f);
        fclose(f);
 
        options.savedata_type = TOX_SAVEDATA_TYPE_TOX_SAVE;
        options.savedata_data = savedata;
        options.savedata_length = fsize;
 
        tox = tox_new(&options, NULL);
 
        free(savedata);
    } else {
        tox = tox_new(&options, NULL);
    }
 
    return tox;
}
 
void update_savedata_file(const Tox *tox)
{
    size_t size = tox_get_savedata_size(tox);
    char *savedata = malloc(size);
    tox_get_savedata(tox, savedata);
 
    FILE *f = fopen(savedata_tmp_filename, "wb");
    fwrite(savedata, size, 1, f);
    fclose(f);
 
    rename(savedata_tmp_filename, savedata_filename);
 
    free(savedata);
}
 
void bootstrap(Tox *tox)
{
    DHT_node nodes[] =
    {
        {"144.76.60.215",   33445, "04119E835DF3E78BACF0F84235B300546AF8B936F035185E2A8E9E0A67C8924F", {0}},
        {"23.226.230.47",   33445, "A09162D68618E742FFBCA1C2C70385E6679604B2D80EA6E84AD0996A1AC8A074", {0}},
        {"178.21.112.187",  33445, "4B2C19E924972CB9B57732FB172F8A8604DE13EEDA2A6234E348983344B23057", {0}},
        {"195.154.119.113", 33445, "E398A69646B8CEACA9F0B84F553726C1C49270558C57DF5F3C368F05A7D71354", {0}},
        {"192.210.149.121", 33445, "F404ABAA1C99A9D37D61AB54898F56793E1DEF8BD46B1038B9D822E8460FAB67", {0}}
    };
 
    for (size_t i = 0; i < sizeof(nodes)/sizeof(DHT_node); i ++) {
        sodium_hex2bin(nodes[i].key_bin, sizeof(nodes[i].key_bin),
                       nodes[i].key_hex, sizeof(nodes[i].key_hex)-1, NULL, NULL, NULL);
        tox_bootstrap(tox, nodes[i].ip, nodes[i].port, nodes[i].key_bin, NULL);
    }
}
 
void print_tox_id(Tox *tox)
{
    uint8_t tox_id_bin[TOX_ADDRESS_SIZE];
    tox_self_get_address(tox, tox_id_bin);
 
    char tox_id_hex[TOX_ADDRESS_SIZE*2 + 1];
    sodium_bin2hex(tox_id_hex, sizeof(tox_id_hex), tox_id_bin, sizeof(tox_id_bin));
 
    for (size_t i = 0; i < sizeof(tox_id_hex)-1; i ++) {
        tox_id_hex[i] = toupper(tox_id_hex[i]);
    }
 
    printf("Tox ID: %s\n", tox_id_hex);
}
 
void friend_request_cb(Tox *tox, const uint8_t *public_key, const uint8_t *message, size_t length,
                                   void *user_data)
{
    tox_friend_add_norequest(tox, public_key, NULL);
 
    update_savedata_file(tox);
}
 
void friend_message_cb(Tox *tox, uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t *message,
                                   size_t length, void *user_data)
{
	if (strcmp(message, "?send") == 0){

		    printf("hello\n");
                    // Mit geratenen parameter. Kein Plan welche dahin müssen. HELP
                    tox_file_send(tox, friend_number, 65536, 4, "0",
                       "test.txt", 8, NULL);
	    }
    tox_friend_send_message(tox, friend_number, type, message, length, NULL);
}

/////////////////
// File Receiving 
void file_recv_cb(Tox *tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size,
                                      const uint8_t *filename, size_t filename_length, void *user_data)
{
    // This parts is responsible for accepting the file send coming from a friend.
    // After this file chunks can be handled
    printf("%s", "HIER\n");
    tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME, NULL);
}

void file_recv_chunk_cb(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position,
                                            const uint8_t *data, size_t length, void *user_data)
{
    // THE PROBLEM writing data chunks coming from data to a file
    if ( length != 0 ) {
    printf("%s", "Hello\n");
    printf("%c\n", data);
    printf("%c\n", &data);
    printf("%i\n", position);
    printf("%i\n", length);
    printf("%i\n", user_data);
    printf("%i\n", &user_data);

//    incoming_file_callback_chunk(tox, friend_number, file_number, position, data, length, user_data);

    FILE *fp;
    char str[] = "This is tutorialspoint.com";

    fp = fopen( "file.txt" , "ab" );
    fwrite(data , 1 , 4 , fp );

    fclose(fp);
    }

}
// END File Receiving
/////////////////////

///////////////
// File Sending
void file_chunk_request_cb(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position,
                                       size_t length, void *user_data)
{
    // THE PROBLEM open a file, split it in chunks and send every chunk. Look at the python version to better understand.
    printf("%s", "Hello2\n");
/*    FILE *f = fopen(savedata_filename, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
 
        char *savedata = malloc(fsize);
 
        fread(savedata, fsize, 1, f);
        fclose(f);
 
        options.savedata_type = TOX_SAVEDATA_TYPE_TOX_SAVE;
        options.savedata_data = savedata;
        options.savedata_length = fsize;
 
        tox = tox_new(&options, NULL);
 
        free(savedata);

*/
    FILE * fp;
    fp = fopen ("file.txt", "rb");
    // some data variable
   
    fclose(fp);
//tox_file_send_chunk(tox, friend_number, file_number, position, const uint8_t *data,
//                         size_t length, TOX_ERR_FILE_SEND_CHUNK *error);
}
// END File Sending
///////////////////

void self_connection_status_cb(Tox *tox, TOX_CONNECTION connection_status, void *user_data)
{
    switch (connection_status) {
        case TOX_CONNECTION_NONE:
            printf("Offline\n");
            break;
        case TOX_CONNECTION_TCP:
            printf("Online, using TCP\n");
            break;
        case TOX_CONNECTION_UDP:
            printf("Online, using UDP\n");
            break;
    }
}
 
int main()
{
    Tox *tox = create_tox();
 
    const char *name = "Echo Bot";
    tox_self_set_name(tox, name, strlen(name), NULL);
 
    const char *status_message = "Echoing your messages";
    tox_self_set_status_message(tox, status_message, strlen(status_message), NULL);
 
    bootstrap(tox);
 
    print_tox_id(tox);
 
    tox_callback_friend_request(tox, friend_request_cb, NULL);
    tox_callback_friend_message(tox, friend_message_cb, NULL);
 
    tox_callback_self_connection_status(tox, self_connection_status_cb, NULL);
 
    update_savedata_file(tox);
 
    // Here begins the file transfer part. No idea why NULL. Because of error msg?
    tox_callback_file_recv(tox, incoming_file_callback_request, NULL);   // Eigentlich steht da wo jetzt NULL ist *user_data. Ist das wichtig? HELP
    // OLD tox_callback_file_recv(tox, file_recv_cb, NULL);
    tox_callback_file_recv_chunk(tox, file_recv_chunk_cb, NULL);
    tox_callback_file_chunk_request(tox, file_chunk_request_cb, NULL);

    while (1) {
        tox_iterate(tox);
        usleep(tox_iteration_interval(tox) * 1000);
    }
 
    tox_kill(tox);
 
    return 0;
}
