python3 -c "
from PIL import Image
import numpy as np
img = Image.open('noise.png').convert('L')
data = np.array(img, dtype=np.uint8)
with open('noise_data.h', 'w') as f:
    f.write('#ifndef NOISE_DATA_H\n#define NOISE_DATA_H\n\n')
    f.write('#define NOISE_WIDTH {}\n'.format(data.shape[1]))
    f.write('#define NOISE_HEIGHT {}\n'.format(data.shape[0]))
    f.write('static const uint8_t noise_data[] = {\n')
    for y in range(data.shape[0]):
        f.write('    ' + ','.join(str(v) for v in data[y]) + ',\n')
    f.write('};\n\n#endif\n')
"