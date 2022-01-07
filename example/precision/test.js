//@ama require('./augment.ama.js')(ParseCurrentFile(),{error:1e-6,samples:1000});
function dot(x0, y0, x1, y1) {
	return x0 * x1 + y0 * y1;
}

let d0 = dot(0.1, 0.03, -0.06, 0.2);
if (d0 > 0) {console.log("d0 is positive");}
let d1 = dot(0.06, 0.2, -0.1, 0.03);
if (d1 < 0) {console.log("d1 is negative");}
if (d0 == d1) {console.log("d0 == d1");}
