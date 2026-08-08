min_ram = 16
min_vram = 4
current_ram = 16
current_vram = 4

if current_ram >= min_ram and current_vram >= min_vram:
    print("Your computer meets the minimum requirements. The game should run.")
else:
    print("Your computer does not meet the minimum requirements. The game may not run smoothly.")