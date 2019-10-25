const User = require('../models/user.model.js');

//Create and Save new User
exports.create = (req, res) => {
    //Validate request
    if(!req.body.userName) {
        return res.status(400).send({
            message: "Username cannot be empty"
        });
    }

    //Create user
    const user = new User({
        userName: req.body.userName,
        testData: req.body.testData
    });

    //Save User in database
    user.save()
    .then(data => {
        res.send(data);
    }).catch(err => {
        res.status(500).send({
            message: err.message || "Some error occured while creataing the User."
        });
    });
};

//Retrieve and return all users from the database
exports.findAll = (req, res) => {
    User.find()
    .then(users => {
        res.send(users);
    }).catch(err => {
        res.status(500).send({
            message: err.message || "Some error occurred while retrieving notes."
        });
    });
};

//Find a single user with a userId
exports.findOne = (req, res) => {
    User.findById(req.params.userId)
    .then(user => {
        if(!user) {
            return res.status(404).send({
                message: "User not found with id " + req.params.userId
            });
        }
        res.send(user);
    }).catch(err => {
        if(err.king === "ObjectId") {
            return res.status(404).send({
                message: "User not found with id " + req.params.userId
            });
        }
        return res.status(500).send({
            message: "Error retrieving user with id " + req.params.userId
        });
    });

};

//Update a user identified by the userId in the request
exports.update = (req, res) => {
    //Validate Request
    if(!req.body.userName) {
        return res.status(400).send({
            message: "Username cannot be empty"
        });
    }

    User.findByIdAndUpdate(req.params.userId, {
        userName: req.body.userName,
        testData: req.body.testData
    }, {new: true})
    .then(note => {
        if(!note) {
            return res.status(404).send({
                message: "User not found with id " + req.params.userId
            });
        }
        res.send(user);
    }).catch(err => {
        if(err.king === "ObjectId") {
            return res.status(404).send({
                message: "User not found with id " + req.params.userId
            });
        }
        return res.status(500).send({
            message: "Error updating user with id " + req.params.userId
        });
    });
};

//Delete a user with the specified userId in the request
exports.delete = (req, res) => {
    User.findByIdAndRemove(req.params.userId)
    .then(user => {
        if(!user) {
            return res.status(404).send({
                message: "User not found with id " + req.params.userId
            });
        }
        res.send({message: "User deleted successfully!"});
    }).catch(err => {
        if(err.kind === "ObjectId" || err.name === "NotFound") {
            return res.status(404).send({
                message: "User not found with id " + req.params.userId
            });
        }
        return res.status(500).send({
            message: "Could not delete user with id " + req.params.noteId
        });
    });

};

