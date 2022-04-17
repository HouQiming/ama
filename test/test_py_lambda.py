if not self.store.containsSession(recipient_id, 1):
    self.getKeysFor([node["to"]], lambda successJids, b: self.sendToContact(node) if len(successJids) == 1 else self.toLower(node), lambda: self.toLower(node))
