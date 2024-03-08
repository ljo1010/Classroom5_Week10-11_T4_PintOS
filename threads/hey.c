
#define f 1<<14
#define f_2 1<<13

#define CONVERT_N_X(n) (int)n*f
#define CONVERT_X_N_ZERO(x) (int32_t)x/f
#define CONVERT_X_N_PLUS(x) ((int32_t)x+f_2)/f
#define CONVERT_X_N_MINUS(x) ((int32_t)x-f_2)/f

#define ADD_X_Y(x,y) (int32_t)x+(int32_t)y 
#define SUB_X_Y(x,y) (int32_t)x-(int32_t)y

#define ADD_X_N(x,n) (int32_t)x+(int)n*f
#define SUB_X_N(x,n) (int32_t)x-(int)n*f

#define MULTI_X_Y(x,y) ((int64_t)x)*(int)y/f
#define MULTI_X_N(x,n) (int32_t)x*n 

#define DIVI_X_Y(x,y) ((int64_t)x)*f/y 
#define DIVI_X_N(x,n) (int32_t)x/n 

#define LOAD_AV_1 DIVI_X_Y(CONVERT_X_N_PLUS(59),CONVERT_X_N_PLUS(60))
#define LOAD_AV_2 DIVI_X_Y(CONVERT_X_N_PLUS(1),CONVERT_X_N_PLUS(60))

int convert_x_n(const signed int x){
    int value;
    if(x> 0){
        value = (x+f_2)/f;
    }
    else if(x<0){
        value = (x-f_2)/f;
    }
    else{
        value = x/f;
    }
    return value ;
}