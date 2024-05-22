import subprocess
import time

# Запускаем объектный файл в другом процессе
command = ['./client', '--ip-address', '127.0.0.1', '--port', '123345']
# command = ['./reverse']
process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

join_message = "/join channel3\n"
process.stdin.write(join_message)
process.stdin.flush()

for i in range(5):
    # Отправляем сообщение в стандартный ввод процесса
    process.stdin.write('/read\n')
    process.stdin.flush()
    
    # Читаем ответ из stdout
    response = process.stdout.readline().strip()
    print(f"Ответ: {response}")
    
    # Пауза в 1 секунду
    time.sleep(1)

process.stdin.write('/exit\n')
process.stdin.flush()

# Завершение процесса C после отправки всех сообщений
process.stdin.close()
process.wait()
