import os
from PIL import Image

def generate_bitmaps():
    icon_path = 'resources/app_icon.png'
    if not os.path.exists(icon_path):
        print(f"Error: {icon_path} not found.")
        return

    # Load logo
    logo = Image.open(icon_path)

    # 1. Welcome Page Bitmap (164 x 314)
    # Dark modern background gradient (slate/dark theme)
    welcome_bg = Image.new('RGB', (164, 314), color=(20, 20, 25))
    
    # Create a nice vertical gradient
    for y in range(314):
        # Gradient from #0d0d11 to #252530
        r = int(13 + (37 - 13) * (y / 314.0))
        g = int(13 + (37 - 13) * (y / 314.0))
        b = int(17 + (48 - 17) * (y / 314.0))
        for x in range(164):
            welcome_bg.putpixel((x, y), (r, g, b))

    # Resize logo to fit nicely (e.g. width of 110)
    logo_w = 110
    logo_h = int(logo.height * (logo_w / logo.width))
    logo_resized = logo.resize((logo_w, logo_h), Image.Resampling.LANCZOS)
    
    # Center the logo horizontally and vertically
    paste_x = (164 - logo_w) // 2
    paste_y = (314 - logo_h) // 2
    
    # Paste using alpha channel as mask
    if logo_resized.mode == 'RGBA':
        welcome_bg.paste(logo_resized, (paste_x, paste_y), logo_resized)
    else:
        welcome_bg.paste(logo_resized, (paste_x, paste_y))
        
    welcome_bg.save('resources/installer_welcome.bmp', 'BMP')
    print("Generated resources/installer_welcome.bmp")

    # 2. Header Page Bitmap (150 x 57)
    # White background to blend with the white NSIS header
    header_bg = Image.new('RGB', (150, 57), color=(255, 255, 255))
    
    # Resize logo to fit the header height (e.g. height of 48)
    logo_h_h = 48
    logo_h_w = int(logo.width * (logo_h_h / logo.height))
    logo_h_resized = logo.resize((logo_h_w, logo_h_h), Image.Resampling.LANCZOS)
    
    # Position on the right side of the 150px wide bitmap
    paste_h_x = 150 - logo_h_w - 10
    paste_h_y = (57 - logo_h_h) // 2
    
    if logo_h_resized.mode == 'RGBA':
        header_bg.paste(logo_h_resized, (paste_h_x, paste_h_y), logo_h_resized)
    else:
        header_bg.paste(logo_h_resized, (paste_h_x, paste_h_y))
        
    header_bg.save('resources/installer_header.bmp', 'BMP')
    print("Generated resources/installer_header.bmp")

if __name__ == '__main__':
    generate_bitmaps()
