#pragma once
#include <GLFW/glfw3.h>
#include <string>
namespace nuc {class Window{GLFWwindow* h_{};int w_,hgt_;static void resized(GLFWwindow*,int,int);public:Window(int,int,const std::string&,bool);~Window();Window(const Window&)=delete;Window& operator=(const Window&)=delete;bool should_close()const;void poll();void swap();void close();GLFWwindow* native()const{return h_;}int width()const{return w_;}int height()const{return hgt_;}};}
