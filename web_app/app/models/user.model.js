const mongoose = require('mongoose');

const UserSchema = mongoose.Schema({
    userName: String,
    testData: [{test1: Number,
        test2: Number,
        test3: Number,
        test4: Number,
        test5: Number,
    }]},    {
            timestamps: true
});

module.exports = mongoose.model('User', UserSchema)