#include <stdio.h>
#include <sqlite3.h>
#include <openssl/sha.h>
#include <string.h>

#define HASH_HEX_LEN (SHA256_DIGEST_LENGTH*2+1)
#define MAX_INPUT 256

void bytes_to_hex(const unsigned char *value, size_t valueLen
, char *hexValue)
{
static const char hex[] = "0123456789abcdef";
for (size_t i=0; i<valueLen; i++)
{
hexValue[i*2] = hex[(value[i] >> 4) & 0xF];
hexValue[i*2 + 1] = hex[value[i] & 0xF];
}
hexValue[valueLen*2] = '\0';
}

void sha256_hash(const char *password , char *passOutput)
{
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256((const unsigned char *)password, 
	strlen(password), hash);
	bytes_to_hex(hash, SHA256_DIGEST_LENGTH, passOutput);
}

int main()
{

	sqlite3 *db;

	int rc = sqlite3_open("test.db", &db);
	if (rc)
	{
		printf("Can't open datbase :( %s\n", sqlite3_errmsg(db));
		return 1;
		
	}
	else{
        printf("opened database successfully :) \n");
	}
	
	char *err_msg = 0;
	const char *sql = "CREATE TABLE IF NOT EXISTS Users(Id INT , Name TEXT);";

	rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
	if(rc != SQLITE_OK){
		printf("Error creating Table :( \n");
		sqlite3_free(err_msg);
	}
	else{
		printf("Table created successfully :) \n");
	}
	char username[MAX_INPUT], hash_hex[HASH_HEX_LEN];
	printf("Enter username: ");
	username[strcspn(username , "\n")] = '\0';
	sha256_hash(username, hash_hex);
	char *sql_insert = sqlite3_mprintf(
	"INSERT INTO Users(Id, Name) VALUES (1, '%q');",
        hash_hex);

	rc = sqlite3_exec(db, sql_insert, 0, 0, &err_msg);
	if(rc != SQLITE_OK){
		printf("ERROR INSERTING DATA :( \n");
		sqlite3_free(err_msg);
	}
	else{
		printf("DATA INSERTED SUCCESSFULLY :) \n");
	}

	int callback(void *NotUsed, int argc, char **argv, char **azColName)
	{
for (int i=1;i<argc;i++)
{
	printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
}
printf("\n");
return 0;
	}

const char *sql_select = "SELECT * FROM Users;";
rc=sqlite3_exec(db,sql_select, callback, 0 , &err_msg);

if (rc != SQLITE_OK)
{
	printf("SQL error \n");
	sqlite3_free(err_msg);
}
	sqlite3_close(db);
	printf("database closed.\n");

	return 0;
}

