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
    private var selectedSaveSlot = 0
    
    let label = UILabel()
    
    let segmentedControl: UISegmentedControl = {
        let control = UISegmentedControl(items: [
            "0", "1", "2", "3", "4","5", "6", "7", "8", "9", "10"
        ])
        return control
    }()
    
    let actionButton: UIButton = {
        let button = UIButton(type: .custom)
        button.layer.borderWidth = 1
        button.layer.borderColor = UIColor.white.cgColor
        button.contentEdgeInsets = UIEdgeInsets(top: 10, left: 10, bottom: 10, right: 10)
        return button
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
        update()
    }
    
    func setupView() {
        view.backgroundColor = UIColor.black.withAlphaComponent(0.8)
        view.addSubview(label)
        view.addSubview(segmentedControl)
        view.addSubview(actionButton)
        label.textColor = .white
        let tap = UITapGestureRecognizer(target: self, action: #selector(didTapOutside(_:)))
        view.addGestureRecognizer(tap)
        actionButton.addTarget(self, action: #selector(didTapButton(_:)), for: .touchUpInside)
    }
    
    func setupConstraints() {
        label.translatesAutoresizingMaskIntoConstraints = false
        segmentedControl.translatesAutoresizingMaskIntoConstraints = false
        actionButton.translatesAutoresizingMaskIntoConstraints = false
        let constraints = [
            label.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            label.centerYAnchor.constraint(equalTo: view.centerYAnchor, constant: -40),
            segmentedControl.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            segmentedControl.centerYAnchor.constraint(equalTo: view.centerYAnchor, constant: 20),
            actionButton.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            actionButton.centerYAnchor.constraint(equalTo: view.centerYAnchor, constant: 40),
        ]
        NSLayoutConstraint.activate(constraints)
    }
    
    private func update() {
        label.text = "Select a state to \(action.name):"
        actionButton.setTitle("\(action.name)", for: .normal)
    }
    
    @objc func didTapButton(_ sender: UIButton) {
        let slot = UInt32(segmentedControl.selectedSegmentIndex)
        switch action {
        case .save:
            delegate?.saveState(position: slot)
        case .load:
            delegate?.loadState(position: slot)
        }
        view.isHidden = true
    }
    
    @objc private func didTapOutside(_ sender: UITapGestureRecognizer) {
        view.isHidden = true
    }
}
