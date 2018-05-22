import numpy as np
import scipy.io as sio
import numpy as np
from docopt import docopt


def main():
    args = docopt("""
    Usage:
        convertTXT.py <name.txt> <name.np>
    """)
    
    input_path = args['<name.txt>']
    output_path = args['<name.np>']

    npa = np.loadtxt(input_path)
    np.save(output_path, npa)
    
    
    print(npa.shape)

if __name__ == '__main__':
    main()
