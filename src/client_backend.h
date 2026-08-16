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
} client_data;

client_data init(const char *ip, unsigned int port);
void close_client(client_data);

void create_account(client_data *, const char *username, const char *password);
void login(client_data *, const char *username, const char *password);
void logout(client_data *);
void save_login_info(const char *username);
const char **load_login_info();

const char **search_user(client_data *,
                         const char *search_str); // prefix search
message_s get_chat_history(client_data *, const char *user);
void send_message(client_data *, const char *user, const char *message);
void delete_message(client_data *, const char *user, size_t message_id);

bool check_ping(client_data *);
#endif
