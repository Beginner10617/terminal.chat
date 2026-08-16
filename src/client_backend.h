#ifndef CLIENT_BACKEND
#define CLIENT_BACKEND
#include "storage.h"
#include <openssl/ssl.h>
typedef enum {
  CLIENT_OK,
  CLIENT_NETWORK_ERROR,
  CLIENT_AUTH_FAILED,
  CLIENT_USER_NOT_FOUND,
  CLIENT_USERNAME_TAKEN,
  CLIENT_INVALID_INPUT,
  CLIENT_SERVER_ERROR,
  CLIENT_NOT_LOGGED_IN,
} client_status;

typedef struct {
  SSL *server_ssl;
  int server_fd;
  char *username;
  bool logged_in;
  client_status status;
  char **user_search_results;
  char *current_dm;
  message_s dm_messages;
} client_data;

client_data init(const char *ip, unsigned int port);
void close_client(client_data);

void create_account(client_data *, char *username, const char *password);
void login(client_data *, char *username, const char *password);
void logout(client_data *);
void save_login_info(const char *username);
void load_login_info(); // auto-load username

void search_user(client_data *,
                 const char *search_str); // prefix search
void update_recent_user(client_data *);
void get_chat_history(client_data *, const char *user);
void send_message(client_data *, const char *user, const char *message);
void delete_message(client_data *, const char *user, size_t message_id);

void check_ping(client_data *);
#endif
