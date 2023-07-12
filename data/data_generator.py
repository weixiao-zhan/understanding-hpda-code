import csv
import random
from faker import Faker

PHONE_NUM = 100
ENTRY_NUM = 100

def generate_to_csv(filename):
    with open(filename, mode='w', newline='') as file:
        writer = csv.writer(file)        
        phone_numbers = [fake.phone_number() for _ in range(PHONE_NUM)]
        for _ in range(ENTRY_NUM):
            phone_number = random.choice(phone_numbers)
            writer.writerow([phone_number, fake.latitude(), fake.longitude(), int(fake.date_time_this_year().timestamp())])

if __name__ == "__main__":
    fake = Faker("zh_CN")
    generate_to_csv('phone_geo_time.csv')