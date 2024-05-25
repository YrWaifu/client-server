У меня есть вот такая функция void send_last_channel_messages(int sd, char *channel_name) {
    char log_filename[100];
    snprintf(log_filename, sizeof(log_filename), "../logs/%s.log", channel_name);

    FILE *log_file = fopen(log_filename, "r");
    if (log_file == NULL) {
        perror("Error opening log file");
        send(sd, "No logs available\n", strlen("No logs available\n"), 0);
        return;
    }

    char *lines[MAX_LOG_LINES];
    for (int i = 0; i < MAX_LOG_LINES; i++) {
        lines[i] = (char *)malloc(MAX_LOG_LINE_LENGTH);
        memset(lines[i], 0, MAX_LOG_LINE_LENGTH);
    }

    int count = 0;
    char buffer[MAX_LOG_LINE_LENGTH];
    while (fgets(buffer, sizeof(buffer), log_file) != NULL) {
        strncpy(lines[count % MAX_LOG_LINES], buffer, MAX_LOG_LINE_LENGTH);
        count++;
    }
    fclose(log_file);

    int start = (count > MAX_LOG_LINES) ? (count % MAX_LOG_LINES) : 0;
    int num_lines_to_send = (count < MAX_LOG_LINES) ? count : MAX_LOG_LINES;

    for (int i = 0; i < num_lines_to_send; i++) {
        send(sd, lines[(start + i) % MAX_LOG_LINES], strlen(lines[(start + i) % MAX_LOG_LINES]), 0);
    }

    sleep(1);

    send(sd, "F", 1, 0);

    for (int i = 0; i < MAX_LOG_LINES; i++) {
        free(lines[i]);
    }
}
сделать по - другому.Я хочу все строки записывать в в одно большое сообщение размером 2048(
                 если не помещается в 2048,
                 то таких сообщений может быть несколько)эти строки должны быть вида 00read00 *
                 size *... *size *
#!#message1 #$ #
#!#message2 #$ #
#!#message3 #$ #
                 ...11endread11 Но вместо message1,
    message2 и тп - строки из логов Символы переноса строки не нужны между строками,
    я тебе их поставила для удобности чтения Напиши отдельную функцию для этого,
    которая принимает на вход массив строк из
    логов(message1, message2...) и возвращает массив строк уже кодированных размера 2048 Забыла уточнить,
    в случае, если 2048 символов не хватает и приходится бить на 2 сообщения,
    они получаются в виде 00read00 * size *... *size *#!#message1 #$##!#message2 #$ #... #!#mes sagen #$
            ##!#messagen
        + 1 #$ #11endread11 Вторая строка заполняется нулями до 2048 символа(то есть до конца) А еще,
    в самом начале сообщения между *size *нужно поставить количество сообщений(строк логов)