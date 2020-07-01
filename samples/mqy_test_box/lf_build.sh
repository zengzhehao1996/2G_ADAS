source ../../zephyr-env.sh
CURR_DIR=${PWD}
echo "current dir=" ${CURR_DIR}
echo ">>>>>>>>>cleaning built binaries"
rm -rf build
echo ">>>>>>>>>ok"

echo ">>>>>>>>>building console"
mkdir build
cd build
cmake -GNinja -DBOARD=aidong_linde429v13 ../
ninja
echo ">>>>>>>>>ok"
cd ${CURR_DIR}
