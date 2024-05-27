#include <time.h>

#define MAX_LOG_LINES 7
#define MAX_LOG_LINE_LENGTH 4096
#define MESSAGE_SIZE 2048
#define LINES_OF_ECNODED_MESSAGE 10
#define MAX_OUTPUT_LINE_LENGTH 4096

#define DELIMITER_START "#!#"
#define DELIMITER_END "#$#"
#define READ_START "00read00"
#define READ_END "11endread11"

void format_time(char *time_str);