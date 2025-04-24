x; 
print(::x);
y;
print(::y);

function f() { return ::x;} 

{ print(::f()); } 