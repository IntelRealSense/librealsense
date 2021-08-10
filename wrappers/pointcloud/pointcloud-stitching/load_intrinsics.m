function[intrinsics] = load_intrinsics(filename)
    A = importdata(filename, ',', 1);
    intrinsics = cameraIntrinsics([A.data(3), A.data(4)], [A.data(5), A.data(6)], [A.data(2), A.data(1)]);
