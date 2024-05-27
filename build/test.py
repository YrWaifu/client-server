import subprocess
import time

# Запускаем объектный файл в другом процессе
command = ['./client', '--ip-address', '127.0.0.1', '--port', '8080']
# command = ['./reverse']
process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

process.stdin.write("2\n")
process.stdin.flush()

process.stdin.write("1\n")
process.stdin.flush()

process.stdin.write("1\n")
process.stdin.flush()

join_message = "/join channel3\n"
process.stdin.write(join_message)
process.stdin.flush()

for i in range(5):
    # Отправляем сообщение в стандартный ввод процесса
    process.stdin.write('/read\n')
    process.stdin.flush()
    
    # Читаем ответ из stdout
    response = process.stdout.read().strip()
    print(f"Ответ: {response}")
    
    

process.stdin.write('/exit\n')
process.stdin.flush()

# Завершение процесса C после отправки всех сообщений
process.stdin.close()
process.wait()
