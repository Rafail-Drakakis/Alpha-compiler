while(true){
    function foo(){
        for(;false;){
            while(a){
                (function (){return;});
                b = 9;
                break;
            }
        }
        continue;
    }
}