echo $1
mv $1/goqrandomx/goqrandomx.go $1/goqrandomx/temp.go
sed '/#define intgo swig_intgo/i #cgo LDFLAGS: '$1'/goqrandomx/goqrandomx.so\n#cgo CXXFLAGS: -I'$1'/src -I'$1'/deps/xmr-stak\n' $1/goqrandomx/temp.go > $1/goqrandomx/goqrandomx.go
rm $1/goqrandomx/temp.go

