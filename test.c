#include <stdio.h>
#include <string.h>

int main() {
    char input[100];  // предполагаем, что строка не будет длиннее 100 символов
    char channelname[50];  // предполагаем, что название канала не будет длиннее 50 символов
    char comment[50];  // предполагаем, что комментарий не будет длиннее 50 символов

    // Пример строки ввода
    strcpy(input, "/add_channel channelname \"llololololasdol\"");

    // Ищем начало и конец названия канала
    char *start = strchr(input, ' ') + 1;  // начало названия канала
    char *end = strchr(start, ' ');        // конец названия канала

    // Копируем название канала
    strncpy(channelname, start, end - start);
    channelname[end - start] = '\0';  // добавляем завершающий нулевой символ

    // Ищем начало и конец комментария
    start = strchr(end, '"') + 1;  // начало комментария
    end = strchr(start + 1, '"');  // конец комментария

    // Копируем комментарий
    strncpy(comment, start, end - start);
    comment[end - start] = '\0';  // добавляем завершающий нулевой символ

    // Выводим результат
    printf("Название канала: %s\n", channelname);
    printf("Комментарий: %s\n", comment);

    return 0;
}
