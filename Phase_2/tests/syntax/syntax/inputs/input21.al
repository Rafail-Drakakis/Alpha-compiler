x = 2;
function f(){
    function g(){
        x = 3;
        function p(){
            local x = 4;
        }
    }
    function d(){
        function l(){
            x = 4;
        }
    }
}