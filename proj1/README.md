# How to build

- Run the following commands
```
cd ./ build
cmake ../
make
```
# Running each processor

## Text Processor

```
./textProcessor
```

## Audio Processor

```
./audioProcessor
```
## Image Processor

```
./imageProcessor <path_to_input_image>
```

# Cleaning build files
```
cmake --build ./build/ --target clean
```
