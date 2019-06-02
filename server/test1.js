var net = require('net');


class Base{
    constructor(arg1,arg2){
        this.arg1 = arg1;
        this.arg2 = arg2;
    }

    common1(){
        console.log('common1'+this.arg1);
    }

    common2(){
        console.log('common2'+this.arg2);
    }
}

class base1 extends Base{
    constructor(arg1,arg2,arg3){
        super(arg1,arg2);
        this.arg3 = arg3;
    }

    base1_common1(){
        console.log('base1 !!!!  '+this.arg3);
    }

    base1_common2(){
        console.log('base1 common2... '+this.arg3);
    }
}

user = new base1('a','b','c');
user.common1();
user.common2();
user.base1_common1();
