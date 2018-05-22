import numpy as np
import scipy.io as sio
import numpy as np
from docopt import docopt


def main():
    args = docopt("""
    Usage:
        convertmm.py <name.mm> <name.npz>
    """)
    
    input_path = args['<name.mm>']
    output_path = args['<name.npz>']

    npa = sio.mmread(input_path)
    np.savez_compressed(output_path, npa)
    
    
    print npa.shape

if __name__ == '__main__':
    main()
