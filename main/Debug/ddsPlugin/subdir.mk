################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../ddsPlugin/ddswriter.cpp \
../ddsPlugin/dxt.cpp \
../ddsPlugin/mipmap.cpp 

OBJS += \
./ddsPlugin/ddswriter.o \
./ddsPlugin/dxt.o \
./ddsPlugin/mipmap.o 

CPP_DEPS += \
./ddsPlugin/ddswriter.d \
./ddsPlugin/dxt.d \
./ddsPlugin/mipmap.d 


# Each subdirectory must supply rules for building sources it contributes
ddsPlugin/%.o: ../ddsPlugin/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/include/gtk-2.0 -I/usr/lib/i386-linux-gnu/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/pango-1.0 -I/usr/include/gio-unix-2.0 -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/libpng12 -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

