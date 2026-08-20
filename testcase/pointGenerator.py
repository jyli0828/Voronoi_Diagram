import random

def pointGenerator():
    num_points = int(input("The number of point: "))
    min_val = 0
    max_val = 610
    filename = f'{num_points}point.txt'
    
    with open(filename, 'w', encoding='utf-8') as f:
        f.write(f"{num_points}\n")
        
        for _ in range(num_points):
            x = random.randint(min_val, max_val)
            y = random.randint(min_val, max_val)
            f.write(f"{x} {y}\n")

        f.write("0\n")

pointGenerator()