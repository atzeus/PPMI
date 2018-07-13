import numpy as np
import scipy.io as sio
import scipy.sparse.csr 
import numpy as np
from docopt import docopt


def main():
    args = docopt("""
    Usage:
        convertmm.py <name.mm> <name.npz>
    """)
    
    input_path = args['<name.mm>']
    output_path = args['<name.npz>']

    m = sio.mmread(input_path).tocsr()
    np.savez_compressed(output_path, data=m.data, indices=m.indices, indptr=m.indptr, shape=m.shape)
    
    
    print m.shape

if __name__ == '__main__':
    main()
