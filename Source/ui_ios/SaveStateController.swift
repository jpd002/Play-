//
//  SaveStateController.swift
//  Play
//
//  Created by Yoshi Sugawara on 7/1/21.
//

import Foundation
import UIKit

@objc protocol SaveStateDelegate: AnyObject {
    func saveState(position: UInt32)
    func loadState(position: UInt32)
}

@objc enum SaveStateAction: Int {
    case save, load
    
    var name: String {
        switch self {
        case .save:
            return "Save"
        case .load:
            return "Load"
        }
    }
}

@objcMembers class SaveStateController: UIViewController {
    @objc weak var delegate: SaveStateDelegate?
    @objc var action: SaveStateAction = .save {
        didSet {
            update()
        }
    }
    
    let label = UILabel()
    
    let segmentedControl: UISegmentedControl = {
        let control = UISegmentedControl(items: [
            "0", "1", "2", "3", "4","5", "6", "7", "8", "9", "10"
        ])
        return control
    }()
    
    init() {
        super.init(nibName: nil, bundle: nil)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        setupView()
        setupConstraints()
        segmentedControl.addTarget(self, action: #selector(didSelectStateSlot(_:)), for: .valueChanged)
        update()
    }
    
    func setupView() {
        view.backgroundColor = UIColor.black.withAlphaComponent(0.8)
        view.addSubview(label)
        view.addSubview(segmentedControl)
    }
    
    func setupConstraints() {
        label.translatesAutoresizingMaskIntoConstraints = false
        segmentedControl.translatesAutoresizingMaskIntoConstraints = false
        let constraints = [
            label.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            label.centerYAnchor.constraint(equalTo: view.centerYAnchor, constant: -20),
            segmentedControl.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            segmentedControl.centerYAnchor.constraint(equalTo: view.centerYAnchor, constant: 20)
        ]
        NSLayoutConstraint.activate(constraints)
    }
    
    private func update() {
        label.text = {
            switch action {
            case .save:
                return "Select a state to save:"
            case .load:
                return "Select a state to load:"
            }
        }()
    }
    
    @objc private func didSelectStateSlot(_ sender: UISegmentedControl) {
        let slot = UInt32(sender.selectedSegmentIndex)
        switch action {
        case .save:
            delegate?.saveState(position: slot)
        case .load:
            delegate?.loadState(position: slot)
        }
        view.isHidden = true
    }
}
