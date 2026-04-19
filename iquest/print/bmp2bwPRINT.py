import sys
import os
from PIL import Image

def convert_exact_384(image_path, output_path, var_name="logo_cs"):
    try:
        if not os.path.exists(image_path):
            print(f"Chyba: Soubor {image_path} neexistuje.")
            return

        # Načtení originálu 308x2474
        img = Image.open(image_path).convert('L')
        orig_w, orig_h = img.size

        # Cílová šířka tiskárny (48 bajtů)
        TARGET_WIDTH = 384
        WIDTH_BYTES = 48

        img_bw = img.point(lambda x: 1 if x < 128 else 0, mode='1')
        pixels = list(img_bw.getdata())

        with open(output_path, 'w') as f:
            f.write("#ifndef PLH_QUEST_H\n#define PLH_QUEST_H\n\n#include <stdint.h>\n\n")
            f.write(f"#define PLH_QUEST_WIDTH_PX {TARGET_WIDTH}\n")
            f.write(f"#define PLH_QUEST_WIDTH_BYTES {WIDTH_BYTES}\n")
            f.write(f"#define PLH_QUEST_HEIGHT_PX {orig_h}\n\n")
            f.write(f"const unsigned char {var_name}[] = {{\n")

            for y in range(orig_h):
                f.write("    ")
                for b in range(WIDTH_BYTES):
                    byte = 0
                    for bit in range(8):
                        x = b * 8 + bit
                        # Pokud jsme uvnitř šířky originálu (308 px), vezmeme pixel
                        if x < orig_w:
                            if pixels[y * orig_w + x]:
                                byte |= (1 << (7 - bit))
                    f.write(f"0x{byte:02X}, ")
                f.write("\n")
            f.write("};\n\n#endif\n")

        print(f"Hotovo: Původních {orig_w} px uloženo do {TARGET_WIDTH} px řádků.")
        print(f"Výška: {orig_h} řádků.")

    except Exception as e:
        print(f"Chyba: {e}")

if __name__ == "__main__":
    convert_exact_384("vstupni_obrazek.bmp", "plh_quest.h")
