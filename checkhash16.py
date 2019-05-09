
import os
import json

oso_shaders_dir = "C:\\plugins\\appleseed\\appleseed\\sandbox\\shaders\\appleseed"

shaderparams = {}

import appleseed as asr
q = asr.ShaderQuery()

for root, dir, files in os.walk(oso_shaders_dir):
    for file in files:
        if file.endswith(".oso"):
            filename = os.path.join(root, file)
            q.open(filename)
            name = q.get_shader_name()

            params = []

            num_params = q.get_num_params()

            for i in range(0, num_params):
                pinfo = q.get_param_info(i)

                pname = pinfo['name']

                if pname in params:
                    print "Warning: Name collision found!"
                    print "Shader: %s, parameter: %s" % (name, pname)
                    
                params.append(pname)

                # For Max map and material input parameters
                params.append(pname + "_map")

            shaderparams[name] = params

with open("shaderparams.json", "w") as f:
    json.dump(shaderparams, f)

def pearson_hash16(param_name):
    T = [
        73, 191, 186, 32, 110, 23, 189, 42, 52, 133, 155, 10, 62, 41, 201, 234,
        203, 113, 50, 18, 111, 139, 247, 231, 244, 30, 131, 168, 68, 194, 59, 195,
        172, 159, 118, 99, 115, 13, 29, 141, 21, 98, 219, 235, 19, 107, 83, 31,
        223, 119, 197, 87, 6, 158, 241, 163, 226, 93, 126, 199, 252, 17, 106, 132,
        109, 169, 214, 85, 33, 91, 4, 254, 15, 227, 233, 187, 5, 170, 7, 101,
        24, 116, 108, 153, 45, 250, 217, 64, 36, 149, 206, 105, 27, 138, 150, 210,
        202, 37, 200, 78, 12, 143, 212, 137, 117, 136, 183, 79, 165, 47, 182, 162,
        228, 243, 215, 53, 127, 44, 35, 178, 167, 253, 0, 204, 58, 121, 198, 161,
        135, 54, 232, 43, 242, 176, 16, 84, 154, 80, 177, 76, 3, 82, 171, 100,
        88, 125, 104, 140, 192, 2, 209, 147, 237, 134, 75, 49, 129, 211, 90, 145,
        151, 26, 124, 128, 224, 39, 251, 164, 40, 48, 221, 102, 218, 9, 94, 61,
        103, 28, 181, 89, 229, 156, 246, 69, 51, 205, 57, 184, 120, 70, 213, 114,
        144, 86, 130, 255, 157, 220, 148, 196, 230, 239, 245, 208, 95, 34, 193, 97,
        180, 146, 11, 225, 71, 20, 216, 66, 46, 166, 25, 14, 236, 77, 160, 142,
        38, 1, 8, 190, 72, 123, 222, 179, 63, 122, 248, 175, 60, 174, 112, 249,
        238, 152, 74, 240, 92, 65, 81, 207, 22, 96, 173, 56, 185, 188, 55, 67
        ]

    hh = [0, 0]
    for j in range(0, 2):
        h = T[(ord(param_name[0]) + j) % 256]

        for i in range(1, len(param_name)):
            h = T[h ^ ord(param_name[i])]

        hh[j] = h

    hash = ((hh[0] << 8) | hh[1]) & 0x7fff
    
    # Limit to avoid collision with obsolete IDs
    if hash < 200:
        hash += 200

    return hash

for shader in shaderparams:
    params = shaderparams[shader]

    paramkeys = set()
    for i, param in enumerate(params):
        key = pearson_hash16(param)

        if key in paramkeys:
            print "Warning: Collision found!"
            print "%s, %s, %s" % (key, shader, param)
        else:
            paramkeys.add(i) # Add parameter index to check the collision with obsolete IDs
            paramkeys.add(key)
