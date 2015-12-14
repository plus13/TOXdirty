#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
 
#include <unistd.h>
 
#include <sodium/utils.h>
#include <tox/tox.h>

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

    FILE *fp;
    char str[] = "This is tutorialspoint.com";

    fp = fopen( "file.txt" , "wb" );
    fwrite(&data , 1 , 4 , fp );

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
    // Eigentlich steht da wo jetzt NULL ist *user_data. Ist das wichtig? HELP
    tox_callback_file_recv(tox, file_recv_cb, NULL);
    tox_callback_file_recv_chunk(tox, file_recv_chunk_cb, NULL);
    tox_callback_file_chunk_request(tox, file_chunk_request_cb, NULL);

    while (1) {
        tox_iterate(tox);
        usleep(tox_iteration_interval(tox) * 1000);
    }
 
    tox_kill(tox);
 
    return 0;
}
