import RPi.GPIO as GPIO
import time
import matplotlib.pyplot as plt

GPIO.setmode(GPIO.BCM)

settings = open("settings.txt", "w")

troyka_pin     = 13
comparator_pin = 14
dac_pins       = [8, 11, 7, 1, 0, 5, 12, 6]
led_pins       = [2, 3, 4, 17, 27, 22, 10, 9]


GPIO.setup(dac_pins, GPIO.OUT)
GPIO.setup(led_pins, GPIO.OUT)

GPIO.setup(comparator_pin, GPIO.IN)
GPIO.setup(troyka_pin, GPIO.OUT, initial = 0)

def number_to_bin(num):
    return [int(bit) for bit in bin(num)[2:].zfill(8)]

BASE_VOLTAGE = 3.3
SLEEP_TIME   = 0.003

def adc():   #двоичный поиск алгоритм
    exponent = 128
    number = 0
    while (exponent > 0):
        number += exponent
        GPIO.output(dac_pins, number_to_bin(number))
        time.sleep(SLEEP_TIME)
        if GPIO.input(comparator_pin) == 1:
            number -= exponent
        exponent //= 2
    return number

# Получаем значение плавающего напряжения с контакта напряжения тройки
def getVoltage():
    return adc() / 256 * BASE_VOLTAGE

# Показывать напряжение в диапазоне от 0 до BASE_VOLTAGE на выводах светодиодов
def showVoltage(voltage : float):
    voltage_int = int(voltage * 256 / BASE_VOLTAGE)
    GPIO.output(led_pins, number_to_bin(voltage_int))

data_voltage = []
data_time    = []

MAX_CAPACITOR_VOLTAGE = 2.6
CHARGED_VOLTAGE    = 0.97 * MAX_CAPACITOR_VOLTAGE 
DISCHARGED_VOLTAGE = 0.20 * MAX_CAPACITOR_VOLTAGE

try:
    print("заряжается...")
    start_time = time.time()
    GPIO.output(troyka_pin, 1) # заряжаем конденсатор
    voltage = getVoltage()
    while (voltage < CHARGED_VOLTAGE):
        data_time.append(time.time() - start_time)
        data_voltage.append(voltage)
        showVoltage(voltage)
        time.sleep(0.01)
        voltage = getVoltage()

    charge_points = len(data_voltage)
    a = data_time[-1]
    print(f'время зарядки: {data_time[-1]}')
    print(f'T информ = {data_time[-1] / charge_points}, частота = {charge_points / data_time[-1]}')
    settings.write(f'T информ= {data_time[-1] / charge_points:.2f}, частота = {charge_points / data_time[-1]:.2f}\n')
    settings.write(f'колв-во точек: {charge_points}\n')
    print(f'кол-во точек: {charge_points}')
    print("разряд...")

    GPIO.output(troyka_pin, 0)
    while (voltage > DISCHARGED_VOLTAGE):
        data_time.append(time.time() - start_time)
        data_voltage.append(voltage)
        showVoltage(voltage)
        time.sleep(0.2)
        voltage = getVoltage()
        GPIO.output(dac_pins, 0)

    print(f'Общее время эксперемента: {data_time[-1]:.2f}')
    discharge_points = len(data_time) - charge_points
    settings.write(f'общее время = {data_time[-1]}, частота = {discharge_points / data_time[-1]}\n')
    settings.write(f'кол-во точек: {discharge_points}\n')
    print(f'время разрядки = {data_time[-1] - a}, частота = {discharge_points / data_time[-1]}')
    print(f'кол-во точек: {discharge_points}')

finally:
    GPIO.output(led_pins, 0)
    GPIO.output(dac_pins, 0)
    GPIO.output(troyka_pin, 0)
    GPIO.cleanup()

    
totalPoints = len(data_voltage)

settings.write(f"ADC: {BASE_VOLTAGE/256} V\n")

settings.close()

with (open("data.txt", "w") ) as file:
    for i in range(totalPoints):
        file.write(f'{data_time[i]} {data_voltage[i]}\n')

plt.plot(data_time, data_voltage)
plt.show()
