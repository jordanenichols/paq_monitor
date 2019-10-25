module.exports = (app) => {
    const users = require('../controllers/user.controller.js');

    //Create new User
    app.post('/users', users.create);

    //Retrieve all Users
    app.get('/users', users.findAll);

    //Retrieve a single User with userId
    app.get('/users/:userID', users.findOne);

    //Update a User with userId
    app.put('/users/:userID', users.update);

    //Delete a User with userID
    app.delete('/users/:userID', users.delete);
}