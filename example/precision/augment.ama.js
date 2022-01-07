
module.exports = function(nd_root, options) {
	let error = options.error || 1e-6;
	let samples = options.samples || 100;
	let value_ids = new Map();
	let original_values = [];
	for (let nd of nd_root.FindAll(N_NUMBER)) {
		if (!value_ids.has(nd.data)) {
			value_ids.set(nd.data, original_values.length);
			original_values.push(parseFloat(nd.data));
		}
		nd.ReplaceWith(@(
			(__values[@(nNumber(value_ids.get(nd.data).toString()))])
		));
	}
	let messages = [];
	for (let checkpoint of nd_root.MatchAll(@(console.log(@(Node.MatchAny(N_STRING, 'message')))))) {
		checkpoint.nd.ReplaceWith(@(
			(__hits[@(nNumber(messages.length.toString()))]++)
		));
		messages.push(checkpoint.message.data);
	}
	let __values = new Float64Array(original_values.length);
	let __hits = new Uint32Array(messages.length);
	let ftest = eval(['(function(__hits,__values){', nd_root.toSource(), '})'].join(''));
	for (let i = 0; i < samples; i++) {
		for (let j = 0; j < __values.length; j++) {
			__values[j] = original_values[j] + (Math.random() * 2 - 1) * error;
		}
		ftest(__hits, __values);
	}
	for (let i = 0; i < __hits.length; i++) {
		console.log((__hits[i] / samples * 100).toFixed(1) + '%', messages[i]);
	}
	return nd_root;
}
