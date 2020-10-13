#!/usr/bin/env python
__doc__ = \
"""

Calculate Root-mean-square deviation (RMSD) of Two Molecules Using Rotation
===========================================================================

Calculate Root-mean-square deviation (RMSD) between structure A and B, in XYZ
or PDB format, using transformation and rotation. The order of the atoms *must*
be the same for both structures.

For more information, usage, example and citation read more at
https://github.com/charnley/rmsd

"""

__version__ = '1.2.5'


import numpy as np
import re


# Python 2/3 compatibility
# Make range a iterator in Python 2
try:
    range = xrange
except NameError:
    pass


def kabsch_rmsd(P, Q):
    """
    Rotate matrix P unto Q using Kabsch algorithm and calculate the RMSD.

    Parameters
    ----------
    P : array
        (N,D) matrix, where N is points and D is dimension.
    Q : array
        (N,D) matrix, where N is points and D is dimension.

    Returns
    -------
    rmsd : float
        root-mean squared deviation
    """
    P = kabsch_rotate(P, Q)
    return rmsd(P, Q)


def kabsch_rotate(P, Q):
    """
    Rotate matrix P unto matrix Q using Kabsch algorithm.

    Parameters
    ----------
    P : array
        (N,D) matrix, where N is points and D is dimension.
    Q : array
        (N,D) matrix, where N is points and D is dimension.

    Returns
    -------
    P : array
        (N,D) matrix, where N is points and D is dimension,
        rotated

    """
    U = kabsch(P, Q)

    # Rotate P
    P = np.dot(P, U)
    return P


def kabsch(P, Q):
    """
    The optimal rotation matrix U is calculated and then used to rotate matrix
    P unto matrix Q so the minimum root-mean-square deviation (RMSD) can be
    calculated.

    Using the Kabsch algorithm with two sets of paired point P and Q, centered
    around the centroid. Each vector set is represented as an NxD
    matrix, where D is the the dimension of the space.

    The algorithm works in three steps:
    - a translation of P and Q
    - the computation of a covariance matrix C
    - computation of the optimal rotation matrix U

    http://en.wikipedia.org/wiki/Kabsch_algorithm

    Parameters
    ----------
    P : array
        (N,D) matrix, where N is points and D is dimension.
    Q : array
        (N,D) matrix, where N is points and D is dimension.

    Returns
    -------
    U : matrix
        Rotation matrix (D,D)

    Example
    -----
    TODO

    """

    # Computation of the covariance matrix
    C = np.dot(np.transpose(P), Q)

    # Computation of the optimal rotation matrix
    # This can be done using singular value decomposition (SVD)
    # Getting the sign of the det(V)*(W) to decide
    # whether we need to correct our rotation matrix to ensure a
    # right-handed coordinate system.
    # And finally calculating the optimal rotation matrix U
    # see http://en.wikipedia.org/wiki/Kabsch_algorithm
    V, S, W = np.linalg.svd(C)
    d = (np.linalg.det(V) * np.linalg.det(W)) < 0.0

    if d:
        S[-1] = -S[-1]
        V[:, -1] = -V[:, -1]

    # Create Rotation matrix U
    U = np.dot(V, W)

    return U


def quaternion_rmsd(P, Q):
    """
    Rotate matrix P unto Q and calculate the RMSD

    based on doi:10.1016/1049-9660(91)90036-O

    Parameters
    ----------
    P : array
        (N,D) matrix, where N is points and D is dimension.
    P : array
        (N,D) matrix, where N is points and D is dimension.

    Returns
    -------
    rmsd : float
    """
    rot = quaternion_rotate(P, Q)
    P = np.dot(P, rot)
    return rmsd(P, Q)


def quaternion_transform(r):
    """
    Get optimal rotation
    note: translation will be zero when the centroids of each molecule are the
    same
    """
    Wt_r = makeW(*r).T
    Q_r = makeQ(*r)
    rot = Wt_r.dot(Q_r)[:3, :3]
    return rot


def makeW(r1, r2, r3, r4=0):
    """
    matrix involved in quaternion rotation
    """
    W = np.asarray([
             [r4, r3, -r2, r1],
             [-r3, r4, r1, r2],
             [r2, -r1, r4, r3],
             [-r1, -r2, -r3, r4]])
    return W


def makeQ(r1, r2, r3, r4=0):
    """
    matrix involved in quaternion rotation
    """
    Q = np.asarray([
             [r4, -r3, r2, r1],
             [r3, r4, -r1, r2],
             [-r2, r1, r4, r3],
             [-r1, -r2, -r3, r4]])
    return Q


def quaternion_rotate(X, Y):
    """
    Calculate the rotation

    Parameters
    ----------
    X : array
        (N,D) matrix, where N is points and D is dimension.
    Y: array
        (N,D) matrix, where N is points and D is dimension.

    Returns
    -------
    rot : matrix
        Rotation matrix (D,D)
    """
    N = X.shape[0]
    W = np.asarray([makeW(*Y[k]) for k in range(N)])
    Q = np.asarray([makeQ(*X[k]) for k in range(N)])
    Qt_dot_W = np.asarray([np.dot(Q[k].T, W[k]) for k in range(N)])
    W_minus_Q = np.asarray([W[k] - Q[k] for k in range(N)])
    A = np.sum(Qt_dot_W, axis=0)
    eigen = np.linalg.eigh(A)
    r = eigen[1][:, eigen[0].argmax()]
    rot = quaternion_transform(r)
    return rot


def centroid(X):
    """
    Calculate the centroid from a vectorset X.

    https://en.wikipedia.org/wiki/Centroid
    Centroid is the mean position of all the points in all of the coordinate
    directions.

    C = sum(X)/len(X)

    Parameters
    ----------
    X : array
        (N,D) matrix, where N is points and D is dimension.

    Returns
    -------
    C : float
        centeroid

    """
    C = X.mean(axis=0)
    return C


def rmsd(V, W):
    """
    Calculate Root-mean-square deviation from two sets of vectors V and W.

    Parameters
    ----------
    V : array
        (N,D) matrix, where N is points and D is dimension.
    W : array
        (N,D) matrix, where N is points and D is dimension.

    Returns
    -------
    rmsd : float
        Root-mean-square deviation

    """
    D = len(V[0])
    N = len(V)
    rmsd = 0.0
    for v, w in zip(V, W):
        rmsd += sum([(v[i] - w[i])**2.0 for i in range(D)])
    return np.sqrt(rmsd/N)


def write_coordinates(atoms, V, title=""):
    """
    Print coordinates V with corresponding atoms to stdout in XYZ format.

    Parameters
    ----------
    atoms : list
        List of atomic types
    V : array
        (N,3) matrix of atomic coordinates
    title : string (optional)
        Title of molecule

    """
    N, D = V.shape

    print(str(N))
    print(title)

    for i in range(N):
        atom = atoms[i]
        atom = atom[0].upper() + atom[1:]
        print("{0:2s} {1:15.8f} {2:15.8f} {3:15.8f}".format(
                atom, V[i, 0], V[i, 1], V[i, 2]))


def get_coordinates(filename, fmt):
    """
    Get coordinates from filename in format fmt. Supports XYZ and PDB.

    Parameters
    ----------
    filename : string
        Filename to read
    fmt : string
        Format of filename. Either xyz or pdb.

    Returns
    -------
    atoms : list
        List of atomic types
    V : array
        (N,3) where N is number of atoms

    """
    if fmt == "xyz":
        return get_coordinates_xyz(filename)
    elif fmt == "pdb":
        return get_coordinates_pdb(filename)
    exit("Could not recognize file format: {:s}".format(fmt))


def get_coordinates_pdb(filename):
    """
    Get coordinates from the first chain in a pdb file
    and return a vectorset with all the coordinates.

    Parameters
    ----------
    filename : string
        Filename to read

    Returns
    -------
    atoms : list
        List of atomic types
    V : array
        (N,3) where N is number of atoms

    """
    # PDB files tend to be a bit of a mess. The x, y and z coordinates
    # are supposed to be in column 31-38, 39-46 and 47-54, but this is
    # not always the case.
    # Because of this the three first columns containing a decimal is used.
    # Since the format doesn't require a space between columns, we use the
    # above column indices as a fallback.
    x_column = None
    V = list()
    # Same with atoms and atom naming.
    # The most robust way to do this is probably
    # to assume that the atomtype is given in column 3.
    atoms = list()

    with open(filename, 'r') as f:
        lines = f.readlines()
        for line in lines:
            if line.startswith("TER") or line.startswith("END"):
                break
            if line.startswith("ATOM"):
                tokens = line.split()
                # Try to get the atomtype
                try:
                    atom = tokens[2][0]
                    if atom in ("H", "C", "N", "O", "S", "P"):
                        atoms.append(atom)
                    else:
                        # e.g. 1HD1
                        atom = tokens[2][1]
                        if atom == "H":
                            atoms.append(atom)
                        else:
                            raise Exception
                except:
                        exit("Error parsing atomtype for the following line: \n{0:s}".format(line))

                if x_column == None:
                    try:
                        # look for x column
                        for i, x in enumerate(tokens):
                            if "." in x and "." in tokens[i + 1] and "." in tokens[i + 2]:
                                x_column = i
                                break
                    except IndexError:
                        exit("Error parsing coordinates for the following line: \n{0:s}".format(line))
                # Try to read the coordinates
                try:
                    V.append(np.asarray(tokens[x_column:x_column + 3], dtype=float))
                except:
                    # If that doesn't work, use hardcoded indices
                    try:
                        x = line[30:38]
                        y = line[38:46]
                        z = line[46:54]
                        V.append(np.asarray([x, y ,z], dtype=float))
                    except:
                        exit("Error parsing input for the following line: \n{0:s}".format(line))


    V = np.asarray(V)
    atoms = np.asarray(atoms)
    assert(V.shape[0] == atoms.size)
    return atoms, V


def get_coordinates_xyz(filename):
    """
    Get coordinates from filename and return a vectorset with all the
    coordinates, in XYZ format.

    Parameters
    ----------
    filename : string
        Filename to read

    Returns
    -------
    atoms : list
        List of atomic types
    V : array
        (N,3) where N is number of atoms

    """

    f = open(filename, 'r')
    V = list()
    atoms = list()
    n_atoms = 0

    # Read the first line to obtain the number of atoms to read
    try:
        n_atoms = int(f.readline())
    except ValueError:
        exit("Could not obtain the number of atoms in the .xyz file.")

    # Skip the title line
    f.readline()

    # Use the number of atoms to not read beyond the end of a file
    for lines_read, line in enumerate(f):

        if lines_read == n_atoms:
            break

        atom = re.findall(r'[a-zA-Z]+', line)[0]
        atom = atom.upper()

        numbers = re.findall(r'[-]?\d+\.\d*(?:[Ee][-\+]\d+)?', line)
        numbers = [float(number) for number in numbers]

        # The numbers are not valid unless we obtain exacly three
        if len(numbers) == 3:
            V.append(np.array(numbers))
            atoms.append(atom)
        else:
            exit("Reading the .xyz file failed in line {0}. Please check the format.".format(lines_read + 2))

    f.close()
    atoms = np.array(atoms)
    V = np.array(V)
    return atoms, V


def main():

    import argparse
    import sys

    description = """
Calculate Root-mean-square deviation (RMSD) between structure A and B, in XYZ
or PDB format, using transformation and rotation. The order of the atoms *must*
be the same for both structures.

For more information, usage, example and citation read more at
https://github.com/charnley/rmsd
"""

    epilog = """output:
  Normal - RMSD calculated the straight-forward way, no translation or rotation.
  Kabsch - RMSD after coordinates are translated and rotated using Kabsch.
  Quater - RMSD after coordinates are translated and rotated using quaternions.
"""

    parser = argparse.ArgumentParser(
                    usage='%(prog)s [options] structure_a structure_b',
                    description=description,
                    formatter_class=argparse.RawDescriptionHelpFormatter,
                    epilog=epilog)

    parser.add_argument('-v', '--version', action='version', version='rmsd ' + __version__ + "\nhttps://github.com/charnley/rmsd")

    parser.add_argument('structure_a', metavar='structure_a', type=str, help='Structure in .xyz or .pdb format')
    parser.add_argument('structure_b', metavar='structure_b', type=str)
    parser.add_argument('-o', '--output', action='store_true', help='print out structure A, centered and rotated unto structure B\'s coordinates in XYZ format')
    parser.add_argument('-f', '--format', action='store', help='Format of input files. Valid format are XYZ and PDB', metavar='fmt')

    parser.add_argument('-m', '--normal', action='store_true', help='Use no transformation')
    parser.add_argument('-k', '--kabsch', action='store_true', help='Use Kabsch algorithm for transformation')
    parser.add_argument('-q', '--quater', action='store_true', help='Use Quaternion algorithm for transformation')

    index_group = parser.add_mutually_exclusive_group()
    index_group.add_argument('-n', '--no-hydrogen', action='store_true', help='ignore hydrogens when calculating RMSD')
    index_group.add_argument('-r', '--remove-idx', nargs='+', type=int, help='index list of atoms NOT to consider', metavar='idx')
    index_group.add_argument('-a', '--add-idx', nargs='+', type=int, help='index list of atoms to consider', metavar='idx')


    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(1)

    args = parser.parse_args()

    # As default use all three methods
    if not args.normal and not args.kabsch and not args.quater:
        args.normal = True
        args.kabsch = True
        args.quater = True

    # As default, load the extension as format
    if args.format == None:
        args.format = args.structure_a.split('.')[-1]

    p_atoms, p_all = get_coordinates(args.structure_a, args.format)
    q_atoms, q_all = get_coordinates(args.structure_b, args.format)

    if np.count_nonzero(p_atoms != q_atoms):
        exit("Atoms not in the same order")

    if args.no_hydrogen:
        not_hydrogens = np.where(p_atoms != 'H')
        P = p_all[not_hydrogens]
        Q = q_all[not_hydrogens]

    elif args.remove_idx:
        N, = p_atoms.shape
        index = range(N)
        index = set(index) - set(args.remove_idx)
        index = list(index)
        P = p_all[index]
        Q = q_all[index]

    elif args.add_idx:
        P = p_all[args.add_idx]
        Q = q_all[args.add_idx]

    else:
        P = p_all[:]
        Q = q_all[:]


    # Calculate 'dumb' RMSD
    if args.normal and not args.output:
        normal_rmsd = rmsd(P, Q)
        print("Normal RMSD: {0}".format(normal_rmsd))

    # Create the centroid of P and Q which is the geometric center of a
    # N-dimensional region and translate P and Q onto that center.
    # http://en.wikipedia.org/wiki/Centroid
    Pc = centroid(P)
    Qc = centroid(Q)
    P -= Pc
    Q -= Qc

    if args.output:
        U = kabsch(P, Q)
        p_all -= Pc
        p_all = np.dot(p_all, U)
        p_all += Qc
        write_coordinates(p_atoms, p_all, title="{} translated".format(args.structure_a))
        quit()

    if args.kabsch:
        print("Kabsch RMSD: {0}".format(kabsch_rmsd(P, Q)))

    if args.quater:
        print("Quater RMSD: {0}".format(quaternion_rmsd(P, Q)))


if __name__ == "__main__":
    main()
