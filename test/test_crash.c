/*
@ama
let nd_root=ParseCurrentFile();
console.log(JSON.stringify(nd_root,null,2))
console.log(nd_root.toSource())
*/

uint256 ComputeMerkleRoot(std::vector<uint256> hashes, bool* mutated) {
    bool mutation = false;
    while (hashes.size() > 1) {
        if (mutated) {
            for (size_t pos = 0; pos + 1 < hashes.size(); pos += 2) {
                if (____) mutation = true;
            }
        }
        if (hashes.size() & 1) {
            hashes.push_back(____);
        }
        SHA256D64(hashes[0].begin(), ____, hashes.size() / 2);
        hashes.resize(hashes.size() / 2);
    }
    if (mutated) *mutated = mutation;
    ____
    return hashes[0];
}

uint256 SignWithEd25519Key(uint256 private_key, uint256 hash){
	____
}

