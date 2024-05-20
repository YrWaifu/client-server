import subprocess

# Запускаем исполняемый файл test
process = subprocess.Popen(['./test'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

# Отправляем "LOL" в качестве ввода
process.stdin.write('LOL\n')
process.stdin.flush()  # Сбрасываем буфер ввода

# Читаем вывод в бесконечном цикле
while True:
    output_line = process.stdout.readline().rstrip()  # Считываем строку вывода
    if not output_line:  # Если строка пустая, процесс завершился или перестал выводить
        break
    print(output_line)  # Выводим полученную строку

# Дополнительно: закрываем процесс после окончания работы
process.terminate()
